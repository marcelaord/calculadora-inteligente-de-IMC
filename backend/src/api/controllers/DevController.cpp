#include "api/controllers/DevController.h"

#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>

#include "AppServices.h"
#include "ai/AiEngine.h"
#include "api/HttpResponses.h"
#include "core/BmiCalculator.h"

#include <chrono>
#include <cmath>
#include <cstdint>
#include <ctime>
#include <random>
#include <string>

namespace healthiq::api {

namespace {

// Formatea epoch seconds como "YYYY-MM-DD HH:MM:SS" (UTC). Se pasa como texto
// y se castea en SQL porque to_timestamp(int8) no resuelve en sentencias
// preparadas con el parametro tipado como bigint.
std::string formatIsoUtc(int64_t epochSeconds) {
    const std::time_t t = static_cast<std::time_t>(epochSeconds);
    std::tm tmv{};
#ifdef _WIN32
    gmtime_s(&tmv, &t);
#else
    gmtime_r(&t, &tmv);
#endif
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tmv);
    return buf;
}

}  // namespace

drogon::Task<void> DevController::fillHistory(
    drogon::HttpRequestPtr req,
    std::function<void(const drogon::HttpResponsePtr&)> callback) {
    try {
        const auto userId = req->getAttributes()->get<int64_t>("userId");

        const auto bodyPtr = req->getJsonObject();
        const Json::Value empty(Json::objectValue);
        const Json::Value* body = bodyPtr ? bodyPtr.get() : &empty;

        const int days = (*body).get("days", 30).asInt();
        if (days < 7 || days > 120) {
            callback(badRequest("days debe estar entre 7 y 120."));
            co_return;
        }

        double baseWeight = (*body).get("baseWeightKg", 80.0).asDouble();
        const double heightCm = (*body).get("heightCm", 170.0).asDouble();
        const int activityLevel = (*body).get("activityLevel", 3).asInt();
        const double noiseKg = (*body).get("noiseKg", 0.4).asDouble();
        const std::string trend = (*body).get("trend", "stable").asString();

        if (baseWeight <= 20 || baseWeight > 300 || heightCm < 100 ||
            heightCm > 250) {
            callback(badRequest(
                "baseWeightKg debe estar entre 20 y 300 y heightCm entre 100 y 250."));
            co_return;
        }
        if (activityLevel < 1 || activityLevel > 5) {
            callback(badRequest("activityLevel debe estar entre 1 y 5."));
            co_return;
        }
        if (trend != "stable" && trend != "loss" && trend != "gain") {
            callback(badRequest("trend debe ser stable, loss o gain."));
            co_return;
        }

        const double slopeKgPerDay = trend == "loss"  ? -0.05
                                     : trend == "gain" ? 0.05
                                                       : 0.0;

        // Borra el historial actual para dejar una serie limpia y consistente.
        co_await AppServices::instance().db().client()->execSqlCoro(
            "DELETE FROM health_records WHERE user_id = $1", userId);

        auto db = AppServices::instance().db().client();
        std::mt19937 rng(static_cast<unsigned>(
            std::chrono::system_clock::now().time_since_epoch().count()));
        std::normal_distribution<double> noise(0.0, noiseKg);

        const auto now = std::chrono::system_clock::now();
        const int64_t nowSec = std::chrono::duration_cast<std::chrono::seconds>(
                                   now.time_since_epoch())
                                   .count();

        for (int i = 0; i < days; ++i) {
            const int daysAgo = days - 1 - i;
            const double weight =
                baseWeight + slopeKgPerDay * static_cast<double>(daysAgo) +
                noise(rng);
            const auto bmi =
                core::BmiCalculator::calculate(weight, heightCm);
            const int64_t createdAt = nowSec - static_cast<int64_t>(daysAgo) * 86400;

            std::string note;
            if (daysAgo == days - 1) note = "primera medicion";
            else if (daysAgo == 0) note = "medicion actual";

            co_await db->execSqlCoro(
                "INSERT INTO health_records(user_id, weight_kg, height_cm, "
                "bmi, activity_level, note, created_at) "
                "VALUES($1, $2, $3, $4, $5, $6, $7::timestamptz)",
                userId, weight, heightCm, bmi.bmi, activityLevel, note,
                formatIsoUtc(createdAt));
        }

        // Reentrena el modelo de IA con el historial nuevo.
        ai::AiEngine engine(AppServices::instance().models(),
                            AppServices::instance().records());
        co_await engine.rebuild(userId);
        const auto analysis = co_await engine.analyze(userId);

        Json::Value out(Json::objectValue);
        out["inserted"] = days;
        out["trend"] = trend;
        out["baseWeightKg"] = baseWeight;
        out["confidence"] =
            analysis.get("prediction", Json::Value(Json::objectValue))
                .get("confidence", "");
        out["analysis"] = analysis;
        callback(ok(out));
    } catch (const std::exception& e) {
        LOG_ERROR << "fillHistory: " << e.what();
        callback(serverError());
    }
}

}  // namespace healthiq::api
