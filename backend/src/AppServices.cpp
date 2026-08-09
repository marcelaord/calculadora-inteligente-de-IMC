#include "AppServices.h"

namespace healthiq {

AppServices& AppServices::instance() {
    static AppServices services;
    return services;
}

void AppServices::initJwt(std::string secret) {
    jwt_ = std::make_unique<security::JwtService>(std::move(secret));
}

void AppServices::initDb(const std::string& connInfo, size_t poolSize) {
    db_.init(connInfo, poolSize);
    db_.createSchema();
}

security::JwtService& AppServices::jwt() {
    return *jwt_;
}

database::Db& AppServices::db() {
    return db_;
}

database::UserRepository AppServices::users() {
    return database::UserRepository(db_.client());
}

database::HealthRecordRepository AppServices::records() {
    return database::HealthRecordRepository(db_.client());
}

database::AiModelRepository AppServices::models() {
    return database::AiModelRepository(db_.client());
}

}  // namespace healthiq
