#ifndef HEALTHIQ_API_CONTROLLERS_AUTH_CONTROLLER_H
#define HEALTHIQ_API_CONTROLLERS_AUTH_CONTROLLER_H

#include <drogon/HttpController.h>

namespace healthiq::api {

class AuthController : public drogon::HttpController<AuthController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(AuthController::registerUser, "/api/v1/auth/register",
                  drogon::Post);
    ADD_METHOD_TO(AuthController::login, "/api/v1/auth/login",
                  drogon::Post);
    ADD_METHOD_TO(AuthController::me, "/api/v1/auth/me",
                  drogon::Get, "JwtAuthFilter");
    ADD_METHOD_TO(AuthController::updateMe, "/api/v1/users/me",
                  drogon::Put, "JwtAuthFilter");
    METHOD_LIST_END

    drogon::Task<void> registerUser(drogon::HttpRequestPtr req,
                              std::function<void(const drogon::HttpResponsePtr&)> callback);
    drogon::Task<void> login(drogon::HttpRequestPtr req,
                       std::function<void(const drogon::HttpResponsePtr&)> callback);
    drogon::Task<void> me(drogon::HttpRequestPtr req,
                    std::function<void(const drogon::HttpResponsePtr&)> callback);
    drogon::Task<void> updateMe(drogon::HttpRequestPtr req,
                          std::function<void(const drogon::HttpResponsePtr&)> callback);
};

}  // namespace healthiq::api

#endif
