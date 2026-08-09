#include "api/controllers/DashboardController.h"

#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>

#include "AppServices.h"
#include "ai/AiEngine.h"
#include "api/HttpResponses.h"

#include <stdexcept>

namespace healthiq::api {

drogon::Task<void> DashboardController::summary(
    drogon::HttpRequestPtr req,
    std::function<void(const drogon::HttpResponsePtr&)> callback) {
    try {
        const auto userId = req->getAttributes()->get<int64_t>("userId");
        ai::AiEngine engine(AppServices::instance().models(),
                            AppServices::instance().records());
        const auto data = co_await engine.dashboard(userId);
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
