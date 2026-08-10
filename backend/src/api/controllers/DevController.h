#ifndef HEALTHIQ_API_CONTROLLERS_DEV_CONTROLLER_H
#define HEALTHIQ_API_CONTROLLERS_DEV_CONTROLLER_H

#include <drogon/HttpController.h>

namespace healthiq::api {

// Endpoints de utilidad para desarrollo/demos. Rellenan historial de la
// cuenta autenticada para poder presentar el dashboard con datos reales.
class DevController : public drogon::HttpController<DevController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(DevController::fillHistory, "/api/v1/dev/fill-history",
                  drogon::Post, "JwtAuthFilter");
    METHOD_LIST_END

    drogon::Task<void> fillHistory(
        drogon::HttpRequestPtr req,
        std::function<void(const drogon::HttpResponsePtr&)> callback);
};

}  // namespace healthiq::api

#endif
