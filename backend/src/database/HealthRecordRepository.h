#ifndef HEALTHIQ_DATABASE_HEALTH_RECORD_REPOSITORY_H
#define HEALTHIQ_DATABASE_HEALTH_RECORD_REPOSITORY_H

#include <drogon/utils/coroutine.h>
#include <drogon/orm/DbClient.h>

#include "core/HealthRecord.h"

#include <cstdint>
#include <vector>

namespace healthiq::database {

class HealthRecordRepository {
public:
    explicit HealthRecordRepository(drogon::orm::DbClientPtr db);

    drogon::Task<core::HealthRecord> insert(const core::HealthRecord& record) const;
    drogon::Task<std::vector<core::HealthRecord>> listByUser(int64_t userId,
                                                             size_t limit = 200) const;
    drogon::Task<std::optional<core::HealthRecord>> latest(int64_t userId) const;
    drogon::Task<std::optional<core::HealthRecord>> findById(int64_t userId,
                                                             int64_t id) const;
    drogon::Task<core::HealthRecord> update(const core::HealthRecord& record) const;
    drogon::Task<bool> remove(int64_t userId, int64_t id) const;
    drogon::Task<std::vector<core::HealthRecord>> listChronological(
        int64_t userId) const;

private:
    static core::HealthRecord fromRow(const drogon::orm::Row& row);
    drogon::orm::DbClientPtr db_;
};

}  // namespace healthiq::database

#endif
