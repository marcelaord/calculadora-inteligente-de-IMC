#ifndef HEALTHIQ_DATABASE_AI_MODEL_REPOSITORY_H
#define HEALTHIQ_DATABASE_AI_MODEL_REPOSITORY_H

#include <drogon/utils/coroutine.h>
#include <drogon/orm/DbClient.h>

#include "ai/ModelState.h"

#include <cstdint>
#include <optional>

namespace healthiq::database {

class AiModelRepository {
public:
    explicit AiModelRepository(drogon::orm::DbClientPtr db);

    drogon::Task<std::optional<ai::ModelState>> load(int64_t userId);
    drogon::Task<ai::ModelState> loadOrCreate(int64_t userId,
                                              double weightKg,
                                              double heightCm);
    drogon::Task<void> save(const ai::ModelState& model);

private:
    static ai::ModelState fromRow(const drogon::orm::Row& row);
    drogon::orm::DbClientPtr db_;
};

}  // namespace healthiq::database

#endif
