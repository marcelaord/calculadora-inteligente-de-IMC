#include "database/AiModelRepository.h"

#include <chrono>
#include <stdexcept>
#include <utility>

namespace healthiq::database {

AiModelRepository::AiModelRepository(drogon::orm::DbClientPtr db)
    : db_(std::move(db)) {}

ai::ModelState AiModelRepository::fromRow(const drogon::orm::Row& row) {
    ai::ModelState m;
    m.userId = row["user_id"].as<int64_t>();
    m.slope = row["slope"].as<double>();
    m.intercept = row["intercept"].as<double>();
    m.sampleCount = row["sample_count"].as<int>();
    m.lastWeight = row["last_weight"].as<double>();
    m.lastHeight = row["last_height"].as<double>();
    m.lastBmi = row["last_bmi"].as<double>();
    m.tStart = row["t_start"].as<int64_t>();
    m.sumX = row["sum_x"].as<double>();
    m.sumY = row["sum_y"].as<double>();
    m.sumXX = row["sum_xx"].as<double>();
    m.sumXY = row["sum_xy"].as<double>();
    return m;
}

drogon::Task<std::optional<ai::ModelState>> AiModelRepository::load(
    int64_t userId) {
    auto result = co_await db_->execSqlCoro(
        "SELECT user_id, slope, intercept, sample_count, last_weight, "
        "last_height, last_bmi, t_start, sum_x, sum_y, sum_xx, sum_xy "
        "FROM ai_models WHERE user_id = $1 LIMIT 1",
        userId);
    if (result.empty()) {
        co_return std::nullopt;
    }
    co_return fromRow(result[0]);
}

drogon::Task<ai::ModelState> AiModelRepository::loadOrCreate(
    int64_t userId, double weightKg, double heightCm) {
    auto existing = co_await load(userId);
    if (existing) {
        co_return *existing;
    }
    ai::ModelState nuevo;
    nuevo.userId = userId;
    nuevo.slope = 0.0;
    nuevo.intercept = weightKg;
    nuevo.sampleCount = 1;
    nuevo.lastWeight = weightKg;
    nuevo.lastHeight = heightCm;
    nuevo.lastBmi = 0.0;
    nuevo.tStart = 0;
    nuevo.sumX = 0.0;
    nuevo.sumY = weightKg;
    nuevo.sumXX = 0.0;
    nuevo.sumXY = 0.0;
    co_return nuevo;
}

drogon::Task<void> AiModelRepository::save(const ai::ModelState& model) {
    co_await db_->execSqlCoro(
        "INSERT INTO ai_models(user_id, slope, intercept, sample_count, "
        "last_weight, last_height, last_bmi, t_start, sum_x, sum_y, sum_xx, "
        "sum_xy, last_updated) "
        "VALUES($1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11, $12, now()) "
        "ON CONFLICT (user_id) DO UPDATE SET "
        "slope = EXCLUDED.slope, "
        "intercept = EXCLUDED.intercept, "
        "sample_count = EXCLUDED.sample_count, "
        "last_weight = EXCLUDED.last_weight, "
        "last_height = EXCLUDED.last_height, "
        "last_bmi = EXCLUDED.last_bmi, "
        "t_start = EXCLUDED.t_start, "
        "sum_x = EXCLUDED.sum_x, "
        "sum_y = EXCLUDED.sum_y, "
        "sum_xx = EXCLUDED.sum_xx, "
        "sum_xy = EXCLUDED.sum_xy, "
        "last_updated = now()",
        model.userId, model.slope, model.intercept, model.sampleCount,
        model.lastWeight, model.lastHeight, model.lastBmi, model.tStart,
        model.sumX, model.sumY, model.sumXX, model.sumXY);
    co_return;
}

}  // namespace healthiq::database
