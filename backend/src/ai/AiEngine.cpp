#include "ai/AiEngine.h"

#include <drogon/utils/Utilities.h>

#include "ai/Predictor.h"
#include "ai/Recommender.h"
#include "core/BmiCalculator.h"

#include <chrono>
#include <cmath>

namespace healthiq::ai {

AiEngine::AiEngine(database::AiModelRepository models,
                   database::HealthRecordRepository records)
    : models_(std::move(models)), records_(std::move(records)) {}

int64_t AiEngine::nowEpochDays() {
    return static_cast<int64_t>(
        std::chrono::system_clock::now().time_since_epoch() /
        std::chrono::days(1));
}

drogon::Task<ModelState> AiEngine::learn(int64_t userId, double weightKg,
                                         double heightCm) {
    auto model = co_await models_.loadOrCreate(userId, weightKg, heightCm);
    model = Predictor::learn(std::move(model), weightKg, heightCm, nowEpochDays());
    const auto bmi = core::BmiCalculator::calculate(weightKg, heightCm);
    model.lastBmi = bmi.bmi;
    co_await models_.save(model);
    co_return model;
}

drogon::Task<Json::Value> AiEngine::analyze(int64_t userId) {
    auto model = co_await models_.load(userId);
    auto latest = co_await records_.latest(userId);

    Json::Value out(Json::objectValue);

    if (!model || !latest) {
        out["available"] = false;
        out["message"] = "Aun no hay suficientes datos. Registra tu primer peso.";
        co_return out;
    }

    const auto pred =
        Predictor::predict(*model, latest->weightKg, latest->heightCm, nowEpochDays());
    const auto recs = Recommender::recommend(pred, latest->bmi, latest->activityLevel);

    out["available"] = true;
    out["bmi"] = latest->bmi;
    out["category"] = core::categoryToString(
        core::BmiCalculator::classify(latest->bmi));
    out["currentWeight"] = latest->weightKg;
    out["prediction"]["weight7d"] = pred.weightIn7d;
    out["prediction"]["weight30d"] = pred.weightIn30d;
    out["prediction"]["weight90d"] = pred.weightIn90d;
    out["prediction"]["bmi30d"] = pred.bmiIn30d;
    out["prediction"]["bmi90d"] = pred.bmiIn90d;
    out["prediction"]["trendKgPerMonth"] = pred.trendKgPerMonth;
    out["prediction"]["slope"] = pred.slope;
    out["prediction"]["confidence"] = pred.confidence;

    Json::Value recArray(Json::arrayValue);
    for (const auto& r : recs) {
        Json::Value item(Json::objectValue);
        item["title"] = r.title;
        item["detail"] = r.detail;
        item["priority"] = r.priority;
        item["category"] = r.category;
        recArray.append(item);
    }
    out["recommendations"] = recArray;
    co_return out;
}

drogon::Task<Json::Value> AiEngine::dashboard(int64_t userId) {
    auto latest = co_await records_.latest(userId);
    auto records = co_await records_.listByUser(userId, 90);
    auto analysis = co_await analyze(userId);

    Json::Value out(Json::objectValue);
    out["hasData"] = !records.empty();
    out["user"] = userId;

    if (!records.empty()) {
        double sum = 0.0;
        for (const auto& r : records) {
            sum += r.weightKg;
        }
        out["stats"]["recordCount"] = static_cast<int>(records.size());
        out["stats"]["avgWeight"] = sum / static_cast<double>(records.size());
        out["stats"]["latestBmi"] = latest ? latest->bmi : 0.0;
        out["stats"]["latestWeight"] = latest ? latest->weightKg : 0.0;
    } else {
        out["stats"]["recordCount"] = 0;
    }

    Json::Value history(Json::arrayValue);
    for (auto it = records.rbegin(); it != records.rend(); ++it) {
        Json::Value item(Json::objectValue);
        item["date"] = it->createdAt;
        item["weightKg"] = it->weightKg;
        item["bmi"] = it->bmi;
        history.append(item);
    }
    out["history"] = history;
    out["analysis"] = analysis;
    co_return out;
}

}  // namespace healthiq::ai
