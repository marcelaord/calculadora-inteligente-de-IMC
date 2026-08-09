#include "api/controllers/DashboardController.h"

#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>

#include "AppServices.h"
#include "ai/AiEngine.h"
#include "api/HttpResponses.h"

#include <cmath>
#include <stdexcept>

namespace healthiq::api {

drogon::Task<void> DashboardController::summary(
    drogon::HttpRequestPtr req,
    std::function<void(const drogon::HttpResponsePtr&)> callback) {
    try {
        const auto userId = req->getAttributes()->get<int64_t>("userId");
        ai::AiEngine engine(AppServices::instance().models(),
                            AppServices::instance().records());
        auto data = co_await engine.dashboard(userId);

        const auto usersRepo = AppServices::instance().users();
        const auto user = co_await usersRepo.findById(userId);
        if (user && user->goalWeightKg > 0 &&
            data.isMember("stats") && data["stats"].isMember("latestWeight")) {
            const double current = data["stats"]["latestWeight"].asDouble();
            const double delta = user->goalWeightKg - current;
            Json::Value goal(Json::objectValue);
            goal["goalWeightKg"] = user->goalWeightKg;
            goal["currentWeight"] = current;
            goal["deltaKg"] = delta;
            goal["reached"] = std::abs(delta) < 0.5;
            data["goal"] = goal;
        }

        callback(ok(data));
    } catch (const std::exception& e) {
        LOG_ERROR << "summary: " << e.what();
        callback(serverError());
    }
}

drogon::Task<void> DashboardController::health(
    drogon::HttpRequestPtr req,
    std::function<void(const drogon::HttpResponsePtr&)> callback) {
    Json::Value out(Json::objectValue);
    out["status"] = "ok";
    out["service"] = "healthiq-backend";
    out["version"] = "1.0.0";
    callback(ok(out));
    co_return;
}

}  // namespace healthiq::api
