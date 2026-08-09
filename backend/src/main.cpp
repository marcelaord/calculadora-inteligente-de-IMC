#include <drogon/drogon.h>
#include <trantor/utils/Logger.h>

#include <memory>
#include <string>

#include "AppServices.h"
#include "api/middleware/JwtAuthFilter.h"
#include "config/AppConfig.h"

using namespace healthiq;

namespace {

trantor::Logger::LogLevel parseLogLevel(const std::string& level) {
    if (level == "trace") return trantor::Logger::kTrace;
    if (level == "debug") return trantor::Logger::kDebug;
    if (level == "info") return trantor::Logger::kInfo;
    if (level == "warn") return trantor::Logger::kWarn;
    if (level == "error") return trantor::Logger::kError;
    return trantor::Logger::kInfo;
}

void registerCors() {
    drogon::app().registerHttpResponseCreationAdvice(
        [](const drogon::HttpResponsePtr& resp) {
            resp->addHeader("Access-Control-Allow-Origin", "*");
            resp->addHeader("Access-Control-Allow-Methods",
                            "GET, POST, PUT, DELETE, OPTIONS");
            resp->addHeader("Access-Control-Allow-Headers",
                            "Content-Type, Authorization");
        });

    drogon::app().registerHandlerViaRegex(
        "/api/v1/.*",
        [](const drogon::HttpRequestPtr&,
           std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setStatusCode(drogon::k204NoContent);
            callback(resp);
        },
        {drogon::Options});
}

}  // namespace

int main() {
    const auto cfg = config::AppConfig::fromEnv();

    auto& app = drogon::app();
    app.addListener(cfg.host, cfg.port)
        .setThreadNum(cfg.threadNum)
        .setLogLevel(parseLogLevel(cfg.logLevel))
        .enableCompressedRequest(true);

    registerCors();

    // Autenticacion y base de datos.
    AppServices::instance().initJwt(cfg.jwtSecret);
    const std::string connInfo =
        "host=" + cfg.pgHost +
        " port=" + std::to_string(cfg.pgPort) +
        " dbname=" + cfg.pgDatabase +
        " user=" + cfg.pgUser +
        " password=" + cfg.pgPassword +
        " connect_timeout=10";
    AppServices::instance().initDb(connInfo, 8);

    // Middleware JWT disponible para los controladores.
    app.registerFilter(std::make_shared<api::JwtAuthFilter>());

    LOG_INFO << "HealthIQ backend iniciando en " << cfg.host << ":"
             << cfg.port << " (db=" << cfg.pgDatabase << ")";

    app.run();
    return 0;
}
