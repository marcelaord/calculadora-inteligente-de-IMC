#ifndef HEALTHIQ_API_CONTROLLERS_HEALTH_CONTROLLER_H
#define HEALTHIQ_API_CONTROLLERS_HEALTH_CONTROLLER_H

#include <drogon/HttpController.h>

namespace healthiq::api {

class HealthController : public drogon::HttpController<HealthController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(HealthController::addRecord, "/api/v1/records",
                  drogon::Post, "JwtAuthFilter");
    ADD_METHOD_TO(HealthController::listRecords, "/api/v1/records",
                  drogon::Get, "JwtAuthFilter");
    ADD_METHOD_TO(HealthController::latestRecord, "/api/v1/records/latest",
                  drogon::Get, "JwtAuthFilter");
    METHOD_LIST_END

    drogon::Task<void> addRecord(drogon::HttpRequestPtr req,
                           std::function<void(const drogon::HttpResponsePtr&)> callback);
    drogon::Task<void> listRecords(drogon::HttpRequestPtr req,
                             std::function<void(const drogon::HttpResponsePtr&)> callback);
    drogon::Task<void> latestRecord(drogon::HttpRequestPtr req,
                              std::function<void(const drogon::HttpResponsePtr&)> callback);
};

}  // namespace healthiq::api

#endif
