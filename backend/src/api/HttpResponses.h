#ifndef HEALTHIQ_API_HTTP_RESPONSES_H
#define HEALTHIQ_API_HTTP_RESPONSES_H

#include <drogon/HttpResponse.h>
#include <drogon/HttpTypes.h>

#include <json/json.h>

namespace healthiq::api {

inline Json::Value makeErrorJson(const std::string& message, const std::string& code = "error") {
    Json::Value v(Json::objectValue);
    v["error"] = true;
    v["message"] = message;
    v["code"] = code;
    return v;
}

inline drogon::HttpResponsePtr jsonResponse(drogon::HttpStatusCode status,
                                            const Json::Value& body) {
    auto resp = drogon::HttpResponse::newHttpJsonResponse(body);
    resp->setStatusCode(status);
    return resp;
}

inline drogon::HttpResponsePtr ok(const Json::Value& body) {
    return jsonResponse(drogon::k200OK, body);
}

inline drogon::HttpResponsePtr created(const Json::Value& body) {
    return jsonResponse(drogon::k201Created, body);
}

inline drogon::HttpResponsePtr badRequest(const std::string& message) {
    return jsonResponse(drogon::k400BadRequest, makeErrorJson(message, "invalid_request"));
}

inline drogon::HttpResponsePtr unauthorized(const std::string& message = "No autorizado") {
    return jsonResponse(drogon::k401Unauthorized, makeErrorJson(message, "unauthorized"));
}

inline drogon::HttpResponsePtr forbidden(const std::string& message = "Prohibido") {
    return jsonResponse(drogon::k403Forbidden, makeErrorJson(message, "forbidden"));
}

inline drogon::HttpResponsePtr notFound(const std::string& message = "Recurso no encontrado") {
    return jsonResponse(drogon::k404NotFound, makeErrorJson(message, "not_found"));
}

inline drogon::HttpResponsePtr conflict(const std::string& message) {
    return jsonResponse(drogon::k409Conflict, makeErrorJson(message, "conflict"));
}

inline drogon::HttpResponsePtr serverError(const std::string& message = "Error interno del servidor") {
    return jsonResponse(drogon::k500InternalServerError, makeErrorJson(message, "internal"));
}

}  // namespace healthiq::api

#endif
