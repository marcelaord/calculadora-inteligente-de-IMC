#ifndef HEALTHIQ_SECURITY_PASSWORD_HASHER_H
#define HEALTHIQ_SECURITY_PASSWORD_HASHER_H

#include <string>

namespace healthiq::security {

class PasswordHasher {
public:
    static std::string hash(const std::string& password);
    static bool verify(const std::string& password,
                       const std::string& stored);
};

}  // namespace healthiq::security

#endif
