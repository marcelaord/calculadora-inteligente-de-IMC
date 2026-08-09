#ifndef HEALTHIQ_API_MIDDLEWARE_JWT_AUTH_FILTER_H
#define HEALTHIQ_API_MIDDLEWARE_JWT_AUTH_FILTER_H

#include <drogon/HttpFilter.h>

namespace healthiq::api {

// Middleware de autenticacion JWT. Valida el header "Authorization: Bearer <token>"
// y expone el id de usuario como atributo "userId" de la peticion.
class JwtAuthFilter : public drogon::HttpFilter<JwtAuthFilter, false> {
public:
    const std::string& className() const override {
        static const std::string name{"JwtAuthFilter"};
        return name;
    }

    void doFilter(const drogon::HttpRequestPtr& req,
                  drogon::FilterCallback&& fcb,
                  drogon::FilterChainCallback&& fccb) override;
};

}  // namespace healthiq::api

#endif
