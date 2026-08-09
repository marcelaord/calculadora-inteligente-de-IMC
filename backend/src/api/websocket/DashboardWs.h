#ifndef HEALTHIQ_API_WEBSOCKET_DASHBOARD_WS_H
#define HEALTHIQ_API_WEBSOCKET_DASHBOARD_WS_H

#include <drogon/WebSocketController.h>

#include <json/json.h>

#include <cstdint>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace healthiq::api {

// Canal de tiempo real por usuario. Cuando el motor de IA analiza un nuevo
// registro, se envia un push al dashboard abierto del usuario.
class DashboardWs : public drogon::WebSocketController<DashboardWs> {
public:
    WS_PATH_LIST_BEGIN
    WS_PATH_ADD("/api/v1/ws/dashboard", "JwtAuthFilter");
    WS_PATH_LIST_END

    void handleNewConnection(const drogon::HttpRequestPtr& req,
                             const drogon::WebSocketConnectionPtr& wsConn) override;
    void handleNewMessage(const drogon::WebSocketConnectionPtr& wsConn,
                          std::string&& message,
                          const drogon::WebSocketMessageType& type) override;
    void handleConnectionClosed(const drogon::WebSocketConnectionPtr& wsConn) override;

    static void broadcastToUser(int64_t userId, const Json::Value& payload);

private:
    static void onConnectionClose(int64_t userId,
                                  const drogon::WebSocketConnectionPtr& wsConn);

    inline static std::mutex mutex_;
    inline static std::unordered_map<int64_t, std::vector<drogon::WebSocketConnectionPtr>> connections_;
};

}  // namespace healthiq::api

#endif
