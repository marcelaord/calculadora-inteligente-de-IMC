#ifndef HEALTHIQ_APP_SERVICES_H
#define HEALTHIQ_APP_SERVICES_H

#include <memory>

#include "database/AiModelRepository.h"
#include "database/Db.h"
#include "database/HealthRecordRepository.h"
#include "database/UserRepository.h"
#include "security/JwtService.h"

namespace healthiq {

// Registro central de dependencias compartidas (acceso desde controllers,
// filtros y websockets). Inicializado una sola vez en main.
class AppServices {
public:
    static AppServices& instance();

    void initJwt(std::string secret);
    void initDb(const std::string& connInfo, size_t poolSize);

    security::JwtService& jwt();
    database::Db& db();

    database::UserRepository users();
    database::HealthRecordRepository records();
    database::AiModelRepository models();

private:
    AppServices() = default;
    std::unique_ptr<security::JwtService> jwt_;
    database::Db db_;
};

}  // namespace healthiq

#endif
