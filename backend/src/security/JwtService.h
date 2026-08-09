#ifndef HEALTHIQ_SECURITY_JWT_SERVICE_H
#define HEALTHIQ_SECURITY_JWT_SERVICE_H

#include <cstdint>
#include <optional>
#include <string>

namespace healthiq::security {

class JwtService {
public:
    explicit JwtService(std::string secret);

    std::string createToken(int64_t userId,
                            const std::string& role,
                            long ttlSeconds = 86400) const;

    struct Payload {
        int64_t userId{0};
        std::string role;
        long issuedAt{0};
        long expiresAt{0};
    };

    // Devuelve el payload si la firma es valida y no ha expirado.
    std::optional<Payload> verify(const std::string& token) const;

    static std::string base64UrlEncode(const std::string& data);
    static std::string base64UrlDecode(const std::string& data);

private:
    std::string hmacSha256(const std::string& data) const;
    static std::string nowEpoch();

    std::string secret_;
};

}  // namespace healthiq::security

#endif
