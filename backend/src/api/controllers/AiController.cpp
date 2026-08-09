#include "api/controllers/AiController.h"

#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>

#include "AppServices.h"
#include "ai/AiEngine.h"
#include "api/HttpResponses.h"

#include <stdexcept>

namespace healthiq::api {

drogon::Task<void> AiController::predict(
    drogon::HttpRequestPtr req,
    std::function<void(const drogon::HttpResponsePtr&)> callback) {
    try {
        const auto userId = req->getAttributes()->get<int64_t>("userId");
        ai::AiEngine engine(AppServices::instance().models(),
                            AppServices::instance().records());
        const auto analysis = co_await engine.analyze(userId);
        Json::Value out(Json::objectValue);
        out["prediction"] = analysis.get("prediction", Json::Value(Json::objectValue));
        out["bmi"] = analysis.get("bmi", 0.0);
        out["available"] = analysis.get("available", false);
        out["message"] = analysis.get("message", "");
        callback(ok(out));
    } catch (const std::exception& e) {
        LOG_ERROR << "predict: " << e.what();
        callback(serverError());
    }
}

drogon::Task<void> AiController::recommendations(
    drogon::HttpRequestPtr req,
    std::function<void(const drogon::HttpResponsePtr&)> callback) {
    try {
        const auto userId = req->getAttributes()->get<int64_t>("userId");
        ai::AiEngine engine(AppServices::instance().models(),
                            AppServices::instance().records());
        const auto analysis = co_await engine.analyze(userId);
        Json::Value out(Json::objectValue);
        out["available"] = analysis.get("available", false);
        out["message"] = analysis.get("message", "");
        out["recommendations"] = analysis.get("recommendations", Json::Value(Json::arrayValue));
        callback(ok(out));
    } catch (const std::exception& e) {
        LOG_ERROR << "recommendations: " << e.what();
        callback(serverError());
    }
}

}  // namespace healthiq::api
