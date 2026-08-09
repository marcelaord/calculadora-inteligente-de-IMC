#ifndef HEALTHIQ_DATABASE_USER_REPOSITORY_H
#define HEALTHIQ_DATABASE_USER_REPOSITORY_H

#include <drogon/orm/DbClient.h>
#include <drogon/utils/coroutine.h>

#include "core/HealthRecord.h"

#include <optional>

namespace healthiq::database {

class UserRepository {
public:
    explicit UserRepository(drogon::orm::DbClientPtr db);

    drogon::Task<core::User> create(const std::string& email,
                                    const std::string& name,
                                    const std::string& passwordHash) const;

    drogon::Task<std::optional<core::User>> findByEmail(const std::string& email) const;
    drogon::Task<std::optional<core::User>> findById(int64_t id) const;

    drogon::Task<core::User> setProfile(int64_t id, const std::string& name,
                                        const std::string& email) const;
    drogon::Task<core::User> setPasswordHash(int64_t id,
                                             const std::string& passwordHash) const;
    drogon::Task<core::User> setGoal(int64_t id, double goalWeightKg) const;

private:
    static core::User fromRow(const drogon::orm::Row& row);
    drogon::orm::DbClientPtr db_;
};

}  // namespace healthiq::database

#endif
