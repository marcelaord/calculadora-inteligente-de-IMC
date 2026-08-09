#ifndef HEALTHIQ_DATABASE_DB_H
#define HEALTHIQ_DATABASE_DB_H

#include <memory>

#include <drogon/orm/DbClient.h>

namespace healthiq::database {

class Db {
public:
    Db() = default;
    ~Db() = default;
    Db(const Db&) = delete;
    Db& operator=(const Db&) = delete;

    void init(const std::string& connInfo, size_t poolSize);
    void createSchema();

    drogon::orm::DbClientPtr client();

private:
    drogon::orm::DbClientPtr client_;
};

}  // namespace healthiq::database

#endif
