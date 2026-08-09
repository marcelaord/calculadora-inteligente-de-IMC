#include "api/websocket/DashboardWs.h"

#include <drogon/HttpRequest.h>

#include <algorithm>

#include "AppServices.h"
#include "api/HttpResponses.h"

namespace healthiq::api {

void DashboardWs::handleNewConnection(const drogon::HttpRequestPtr& req,
                                      const drogon::WebSocketConnectionPtr& wsConn) {
    const auto userId = req->getAttributes()->get<int64_t>("userId");
    LOG_INFO << "Dashboard WS conectado (usuario " << userId << ")";

    wsConn->setContext(std::make_shared<int64_t>(userId));

    {
        std::lock_guard<std::mutex> lock(mutex_);
        connections_[userId].push_back(wsConn);
    }

    Json::Value welcome(Json::objectValue);
    welcome["event"] = "connected";
    welcome["message"] = "Canal en tiempo real activo.";
    wsConn->send(welcome.toStyledString());
}

void DashboardWs::handleNewMessage(const drogon::WebSocketConnectionPtr& wsConn,
                                   std::string&& message,
                                   const drogon::WebSocketMessageType& type) {
    (void)message;
    (void)type;
    // El canal es unidireccional (server -> client). Se ignora el mensaje.
    LOG_INFO << "Mensaje WS recibido (ignorado).";
}

void DashboardWs::handleConnectionClosed(
    const drogon::WebSocketConnectionPtr& wsConn) {
    const auto ctx = wsConn->getContext<int64_t>();
    if (!ctx) return;
    const int64_t userId = *ctx;
    LOG_INFO << "Dashboard WS cerrado (usuario " << userId << ")";
    onConnectionClose(userId, wsConn);
}

void DashboardWs::onConnectionClose(
    int64_t userId, const drogon::WebSocketConnectionPtr& wsConn) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = connections_.find(userId);
    if (it == connections_.end()) return;
    auto& vec = it->second;
    vec.erase(std::remove(vec.begin(), vec.end(), wsConn), vec.end());
    if (vec.empty()) {
        connections_.erase(it);
    }
}

void DashboardWs::broadcastToUser(int64_t userId, const Json::Value& payload) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = connections_.find(userId);
    if (it == connections_.end()) return;
    const auto message = payload.toStyledString();
    for (const auto& conn : it->second) {
        if (conn && conn->connected()) {
            conn->send(message);
        }
    }
}

}  // namespace healthiq::api
