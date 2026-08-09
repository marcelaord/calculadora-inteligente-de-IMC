#include "api/middleware/JwtAuthFilter.h"

#include <drogon/HttpRequest.h>

#include "AppServices.h"
#include "api/HttpResponses.h"

namespace healthiq::api {

void JwtAuthFilter::doFilter(const drogon::HttpRequestPtr& req,
                             drogon::FilterCallback&& fcb,
                             drogon::FilterChainCallback&& fccb) {
    const auto& auth = req->getHeader("Authorization");
    std::string token;
    if (auth.size() > 7 && auth.substr(0, 7) == "Bearer ") {
        token = auth.substr(7);
    } else {
        // Soporte para WebSockets desde navegador (query param).
        token = req->getParameter("token");
    }
    if (!token.empty()) {
        const auto payload = AppServices::instance().jwt().verify(token);
        if (payload) {
            req->getAttributes()->insert("userId", payload->userId);
            req->getAttributes()->insert("userRole", payload->role);
            fccb();
            return;
        }
    }
    fcb(unauthorized());
}

}  // namespace healthiq::api
