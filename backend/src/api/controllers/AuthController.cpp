#include "api/controllers/AuthController.h"

#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>

#include "AppServices.h"
#include "api/HttpResponses.h"
#include "security/PasswordHasher.h"

#include <stdexcept>

namespace healthiq::api {

namespace {
bool isValidEmail(const std::string& email) {
    const auto at = email.find('@');
    return at != std::string::npos && at > 0 && at + 1 < email.size();
}
}  // namespace

drogon::Task<void> AuthController::registerUser(
    drogon::HttpRequestPtr req,
    std::function<void(const drogon::HttpResponsePtr&)> callback) {
    try {
        const auto body = req->getJsonObject();
        if (!body) {
            callback(badRequest("El cuerpo debe ser JSON."));
            co_return;
        }
        const auto email = (*body).get("email", "").asString();
        const auto name = (*body).get("name", "").asString();
        const auto password = (*body).get("password", "").asString();

        if (email.empty() || name.empty() || password.size() < 8) {
            callback(badRequest(
                "Campos requeridos: email, name, password (minimo 8 caracteres)."));
            co_return;
        }
        if (!isValidEmail(email)) {
            callback(badRequest("El email no tiene un formato valido."));
            co_return;
        }

        const auto repo = AppServices::instance().users();
        const auto existing = co_await repo.findByEmail(email);
        if (existing) {
            callback(conflict("Ya existe una cuenta con ese email."));
            co_return;
        }

        const auto hash = security::PasswordHasher::hash(password);
        const auto user = co_await repo.create(email, name, hash);

        const auto token = AppServices::instance().jwt().createToken(
            user.id, user.role, /*ttl*/ 86400);

        Json::Value out(Json::objectValue);
        out["token"] = token;
        out["user"]["id"] = user.id;
        out["user"]["email"] = user.email;
        out["user"]["name"] = user.name;
        out["user"]["role"] = user.role;
        callback(created(out));
    } catch (const std::exception& e) {
        LOG_ERROR << "registerUser: " << e.what();
        callback(serverError());
    }
}

drogon::Task<void> AuthController::login(
    drogon::HttpRequestPtr req,
    std::function<void(const drogon::HttpResponsePtr&)> callback) {
    try {
        const auto body = req->getJsonObject();
        if (!body) {
            callback(badRequest("El cuerpo debe ser JSON."));
            co_return;
        }
        const auto email = (*body).get("email", "").asString();
        const auto password = (*body).get("password", "").asString();

        const auto repo = AppServices::instance().users();
        const auto user = co_await repo.findByEmail(email);
        if (!user ||
            !security::PasswordHasher::verify(password, user->passwordHash)) {
            callback(unauthorized("Credenciales invalidas."));
            co_return;
        }

        const auto token = AppServices::instance().jwt().createToken(
            user->id, user->role, /*ttl*/ 86400);

        Json::Value out(Json::objectValue);
        out["token"] = token;
        out["user"]["id"] = user->id;
        out["user"]["email"] = user->email;
        out["user"]["name"] = user->name;
        out["user"]["role"] = user->role;
        callback(ok(out));
    } catch (const std::exception& e) {
        LOG_ERROR << "login: " << e.what();
        callback(serverError());
    }
}

drogon::Task<void> AuthController::me(
    drogon::HttpRequestPtr req,
    std::function<void(const drogon::HttpResponsePtr&)> callback) {
    try {
        const auto userId = req->getAttributes()->get<int64_t>("userId");
        const auto repo = AppServices::instance().users();
        const auto user = co_await repo.findById(userId);
        if (!user) {
            callback(notFound("Usuario no encontrado."));
            co_return;
        }
        Json::Value out(Json::objectValue);
        out["user"]["id"] = user->id;
        out["user"]["email"] = user->email;
        out["user"]["name"] = user->name;
        out["user"]["role"] = user->role;
        out["user"]["createdAt"] = user->createdAt;
        callback(ok(out));
    } catch (const std::exception& e) {
        LOG_ERROR << "me: " << e.what();
        callback(serverError());
    }
}

}  // namespace healthiq::api
