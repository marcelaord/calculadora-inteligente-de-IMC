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

Json::Value userToJson(const core::User& user) {
    Json::Value out(Json::objectValue);
    out["id"] = user.id;
    out["email"] = user.email;
    out["name"] = user.name;
    out["role"] = user.role;
    out["goalWeightKg"] = user.goalWeightKg;
    return out;
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
        out["user"] = userToJson(user);
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
        out["user"] = userToJson(*user);
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
        out["user"] = userToJson(*user);
        out["user"]["createdAt"] = user->createdAt;
        callback(ok(out));
    } catch (const std::exception& e) {
        LOG_ERROR << "me: " << e.what();
        callback(serverError());
    }
}

drogon::Task<void> AuthController::updateMe(
    drogon::HttpRequestPtr req,
    std::function<void(const drogon::HttpResponsePtr&)> callback) {
    try {
        const auto userId = req->getAttributes()->get<int64_t>("userId");
        const auto body = req->getJsonObject();
        if (!body) {
            callback(badRequest("El cuerpo debe ser JSON."));
            co_return;
        }

        const auto repo = AppServices::instance().users();
        const auto current = co_await repo.findById(userId);
        if (!current) {
            callback(notFound("Usuario no encontrado."));
            co_return;
        }

        std::string name = current->name;
        std::string email = current->email;
        double goal = current->goalWeightKg;
        const bool hasNewPassword = (*body).isMember("newPassword") &&
                                    !(*body)["newPassword"].asString().empty();

        if ((*body).isMember("email") || hasNewPassword) {
            const auto currentPassword = (*body).get("currentPassword", "").asString();
            if (currentPassword.empty() ||
                !security::PasswordHasher::verify(currentPassword,
                                                  current->passwordHash)) {
                callback(unauthorized(
                    "Debes ingresar tu contrasena actual para cambiar estos datos."));
                co_return;
            }
        }

        if ((*body).isMember("name")) {
            name = (*body)["name"].asString();
            if (name.empty()) {
                callback(badRequest("El nombre no puede estar vacio."));
                co_return;
            }
        }
        if ((*body).isMember("email")) {
            email = (*body)["email"].asString();
            if (!isValidEmail(email)) {
                callback(badRequest("El email no tiene un formato valido."));
                co_return;
            }
            if (email != current->email) {
                const auto existing = co_await repo.findByEmail(email);
                if (existing && existing->id != userId) {
                    callback(conflict("Ya existe una cuenta con ese email."));
                    co_return;
                }
            }
        }
        if ((*body).isMember("newPassword")) {
            const auto newPassword = (*body)["newPassword"].asString();
            if (newPassword.size() < 8) {
                callback(badRequest("La nueva contrasena debe tener minimo 8 caracteres."));
                co_return;
            }
            const auto hash = security::PasswordHasher::hash(newPassword);
            co_await repo.setPasswordHash(userId, hash);
        }
        if ((*body).isMember("goalWeightKg")) {
            goal = (*body)["goalWeightKg"].asDouble();
            if (goal < 0 || goal > 600) {
                callback(badRequest("goalWeightKg debe estar entre 0 y 600."));
                co_return;
            }
            co_await repo.setGoal(userId, goal);
        }

        core::User updated = *current;
        if (name != current->name || email != current->email) {
            updated = co_await repo.setProfile(userId, name, email);
        }
        updated.goalWeightKg = goal;

        Json::Value out(Json::objectValue);
        out["user"] = userToJson(updated);
        callback(ok(out));
    } catch (const std::exception& e) {
        LOG_ERROR << "updateMe: " << e.what();
        callback(serverError());
    }
}

}  // namespace healthiq::api
