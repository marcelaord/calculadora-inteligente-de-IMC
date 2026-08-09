#ifndef HEALTHIQ_CONFIG_APP_CONFIG_H
#define HEALTHIQ_CONFIG_APP_CONFIG_H

#include <cstdint>
#include <string>

namespace healthiq::config {

struct AppConfig {
    std::string host{"0.0.0.0"};
    uint16_t port{8080};
    size_t threadNum{4};
    std::string logLevel{"info"};

    // PostgreSQL
    std::string pgHost{"localhost"};
    uint16_t pgPort{5432};
    std::string pgDatabase{"healthiq"};
    std::string pgUser{"healthiq"};
    std::string pgPassword{"healthiq"};

    // Seguridad
    std::string jwtSecret{"cambia-este-secreto-en-produccion"};
    long jwtTtlSeconds{86400};

    static AppConfig fromEnv();
};

}  // namespace healthiq::config

#endif
