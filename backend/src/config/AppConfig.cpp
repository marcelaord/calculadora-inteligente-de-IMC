#include "config/AppConfig.h"

#include <cstdlib>

namespace healthiq::config {

namespace {
std::string env(const char* name, const std::string& fallback) {
    const char* v = std::getenv(name);
    return (v && *v) ? std::string(v) : fallback;
}

uint16_t envU16(const char* name, uint16_t fallback) {
    const char* v = std::getenv(name);
    if (!v || !*v) return fallback;
    try {
        const int parsed = std::stoi(v);
        if (parsed > 0 && parsed <= 65535) return static_cast<uint16_t>(parsed);
    } catch (...) {
    }
    return fallback;
}

size_t envSize(const char* name, size_t fallback) {
    const char* v = std::getenv(name);
    if (!v || !*v) return fallback;
    try {
        const long parsed = std::stol(v);
        if (parsed > 0) return static_cast<size_t>(parsed);
    } catch (...) {
    }
    return fallback;
}

long envLong(const char* name, long fallback) {
    const char* v = std::getenv(name);
    if (!v || !*v) return fallback;
    try {
        return std::stol(v);
    } catch (...) {
    }
    return fallback;
}
}  // namespace

AppConfig AppConfig::fromEnv() {
    AppConfig c;
    c.host = env("HEALTHIQ_HOST", c.host);
    c.port = envU16("HEALTHIQ_PORT", c.port);
    c.threadNum = envSize("HEALTHIQ_THREADS", c.threadNum);
    c.logLevel = env("HEALTHIQ_LOG_LEVEL", c.logLevel);

    c.pgHost = env("PGHOST", c.pgHost);
    c.pgPort = envU16("PGPORT", c.pgPort);
    c.pgDatabase = env("PGDATABASE", c.pgDatabase);
    c.pgUser = env("PGUSER", c.pgUser);
    c.pgPassword = env("PGPASSWORD", c.pgPassword);

    c.jwtSecret = env("JWT_SECRET", c.jwtSecret);
    c.jwtTtlSeconds = envLong("JWT_TTL_SECONDS", c.jwtTtlSeconds);
    return c;
}

}  // namespace healthiq::config
