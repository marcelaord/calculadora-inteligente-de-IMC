#ifndef HEALTHIQ_API_CONTROLLERS_AI_CONTROLLER_H
#define HEALTHIQ_API_CONTROLLERS_AI_CONTROLLER_H

#include <drogon/HttpController.h>

namespace healthiq::api {

class AiController : public drogon::HttpController<AiController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(AiController::predict, "/api/v1/ai/predict",
                  drogon::Get, "JwtAuthFilter");
    ADD_METHOD_TO(AiController::recommendations, "/api/v1/ai/recommendations",
                  drogon::Get, "JwtAuthFilter");
    METHOD_LIST_END

    drogon::Task<void> predict(drogon::HttpRequestPtr req,
                         std::function<void(const drogon::HttpResponsePtr&)> callback);
    drogon::Task<void> recommendations(drogon::HttpRequestPtr req,
                                 std::function<void(const drogon::HttpResponsePtr&)> callback);
};

}  // namespace healthiq::api

#endif
