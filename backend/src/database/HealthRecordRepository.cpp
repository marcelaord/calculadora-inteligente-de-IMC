#include "database/HealthRecordRepository.h"

#include <stdexcept>
#include <utility>

namespace healthiq::database {

HealthRecordRepository::HealthRecordRepository(drogon::orm::DbClientPtr db)
    : db_(std::move(db)) {}

core::HealthRecord HealthRecordRepository::fromRow(const drogon::orm::Row& row) {
    core::HealthRecord r;
    r.id = row["id"].as<int64_t>();
    r.userId = row["user_id"].as<int64_t>();
    r.weightKg = row["weight_kg"].as<double>();
    r.heightCm = row["height_cm"].as<double>();
    r.bmi = row["bmi"].as<double>();
    r.activityLevel = row["activity_level"].as<int>();
    r.note = row["note"].as<std::string>();
    r.createdAt = row["created_at"].as<std::string>();
    return r;
}

drogon::Task<core::HealthRecord> HealthRecordRepository::insert(
    const core::HealthRecord& record) const {
    auto result = co_await db_->execSqlCoro(
        "INSERT INTO health_records(user_id, weight_kg, height_cm, bmi, "
        "activity_level, note) "
        "VALUES($1, $2, $3, $4, $5, $6) "
        "RETURNING id, user_id, weight_kg, height_cm, bmi, activity_level, "
        "note, created_at",
        record.userId, record.weightKg, record.heightCm, record.bmi,
        record.activityLevel, record.note);
    if (result.empty()) {
        throw std::runtime_error("No se pudo guardar el registro");
    }
    co_return fromRow(result[0]);
}

drogon::Task<std::vector<core::HealthRecord>>
HealthRecordRepository::listByUser(int64_t userId, size_t limit) const {
    std::vector<core::HealthRecord> records;
    auto result = co_await db_->execSqlCoro(
        "SELECT id, user_id, weight_kg, height_cm, bmi, activity_level, note, "
        "created_at "
        "FROM health_records WHERE user_id = $1 "
        "ORDER BY created_at DESC LIMIT $2",
        userId, static_cast<int64_t>(limit));
    records.reserve(result.size());
    for (const auto& row : result) {
        records.push_back(fromRow(row));
    }
    co_return records;
}

drogon::Task<std::optional<core::HealthRecord>>
HealthRecordRepository::latest(int64_t userId) const {
    auto result = co_await db_->execSqlCoro(
        "SELECT id, user_id, weight_kg, height_cm, bmi, activity_level, note, "
        "created_at "
        "FROM health_records WHERE user_id = $1 "
        "ORDER BY created_at DESC LIMIT 1",
        userId);
    if (result.empty()) {
        co_return std::nullopt;
    }
    co_return fromRow(result[0]);
}

drogon::Task<std::optional<core::HealthRecord>>
HealthRecordRepository::findById(int64_t userId, int64_t id) const {
    auto result = co_await db_->execSqlCoro(
        "SELECT id, user_id, weight_kg, height_cm, bmi, activity_level, note, "
        "created_at "
        "FROM health_records WHERE id = $1 AND user_id = $2 LIMIT 1",
        id, userId);
    if (result.empty()) {
        co_return std::nullopt;
    }
    co_return fromRow(result[0]);
}

drogon::Task<core::HealthRecord> HealthRecordRepository::update(
    const core::HealthRecord& record) const {
    auto result = co_await db_->execSqlCoro(
        "UPDATE health_records "
        "SET weight_kg = $3, height_cm = $4, bmi = $5, activity_level = $6, "
        "note = $7 "
        "WHERE id = $1 AND user_id = $2 "
        "RETURNING id, user_id, weight_kg, height_cm, bmi, activity_level, "
        "note, created_at",
        record.id, record.userId, record.weightKg, record.heightCm, record.bmi,
        record.activityLevel, record.note);
    if (result.empty()) {
        throw std::runtime_error("Registro no encontrado");
    }
    co_return fromRow(result[0]);
}

drogon::Task<bool> HealthRecordRepository::remove(int64_t userId, int64_t id) const {
    auto result = co_await db_->execSqlCoro(
        "DELETE FROM health_records WHERE id = $1 AND user_id = $2 RETURNING id",
        id, userId);
    co_return !result.empty();
}

drogon::Task<std::vector<core::HealthRecord>>
HealthRecordRepository::listChronological(int64_t userId) const {
    std::vector<core::HealthRecord> records;
    auto result = co_await db_->execSqlCoro(
        "SELECT id, user_id, weight_kg, height_cm, bmi, activity_level, note, "
        "created_at, (EXTRACT(EPOCH FROM created_at)::bigint / 86400) AS "
        "epoch_day "
        "FROM health_records WHERE user_id = $1 ORDER BY created_at ASC",
        userId);
    records.reserve(result.size());
    for (const auto& row : result) {
        auto r = fromRow(row);
        r.epochDay = row["epoch_day"].as<int64_t>();
        records.push_back(r);
    }
    co_return records;
}

}  // namespace healthiq::database
