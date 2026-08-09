#ifndef HEALTHIQ_API_CONTROLLERS_DASHBOARD_CONTROLLER_H
#define HEALTHIQ_API_CONTROLLERS_DASHBOARD_CONTROLLER_H

#include <drogon/HttpController.h>

namespace healthiq::api {

class DashboardController : public drogon::HttpController<DashboardController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(DashboardController::summary, "/api/v1/dashboard/summary",
                  drogon::Get, "JwtAuthFilter");
    ADD_METHOD_TO(DashboardController::health, "/api/v1/health",
                  drogon::Get);
    METHOD_LIST_END

    drogon::Task<void> summary(drogon::HttpRequestPtr req,
                         std::function<void(const drogon::HttpResponsePtr&)> callback);
    drogon::Task<void> health(drogon::HttpRequestPtr req,
                        std::function<void(const drogon::HttpResponsePtr&)> callback);
};

}  // namespace healthiq::api

#endif
