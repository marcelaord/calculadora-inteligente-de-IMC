#include "database/UserRepository.h"

#include <drogon/orm/Exception.h>

#include <stdexcept>
#include <utility>

namespace healthiq::database {

UserRepository::UserRepository(drogon::orm::DbClientPtr db) : db_(std::move(db)) {}

constexpr const char* kUserColumns =
    "id, email, name, password_hash, role, created_at, "
    "COALESCE(goal_weight_kg, 0) AS goal_weight_kg";

core::User UserRepository::fromRow(const drogon::orm::Row& row) {
    core::User u;
    u.id = row["id"].as<int64_t>();
    u.email = row["email"].as<std::string>();
    u.name = row["name"].as<std::string>();
    u.passwordHash = row["password_hash"].as<std::string>();
    u.role = row["role"].as<std::string>();
    u.createdAt = row["created_at"].as<std::string>();
    u.goalWeightKg = row["goal_weight_kg"].as<double>();
    return u;
}

drogon::Task<core::User> UserRepository::create(const std::string& email,
                                                const std::string& name,
                                                const std::string& passwordHash) const {
    auto result = co_await db_->execSqlCoro(
        std::string("INSERT INTO users(email, name, password_hash) "
                    "VALUES($1, $2, $3) RETURNING ") +
            kUserColumns,
        email, name, passwordHash);
    if (result.empty()) {
        throw std::runtime_error("No se pudo crear el usuario");
    }
    co_return fromRow(result[0]);
}

drogon::Task<std::optional<core::User>> UserRepository::findByEmail(
    const std::string& email) const {
    auto result = co_await db_->execSqlCoro(
        std::string("SELECT ") + kUserColumns + " FROM users WHERE email = $1 LIMIT 1",
        email);
    if (result.empty()) {
        co_return std::nullopt;
    }
    co_return fromRow(result[0]);
}

drogon::Task<std::optional<core::User>> UserRepository::findById(int64_t id) const {
    auto result = co_await db_->execSqlCoro(
        std::string("SELECT ") + kUserColumns + " FROM users WHERE id = $1 LIMIT 1",
        id);
    if (result.empty()) {
        co_return std::nullopt;
    }
    co_return fromRow(result[0]);
}

drogon::Task<core::User> UserRepository::setProfile(int64_t id,
                                                    const std::string& name,
                                                    const std::string& email) const {
    auto result = co_await db_->execSqlCoro(
        std::string("UPDATE users SET name = $1, email = $2 WHERE id = $3 "
                    "RETURNING ") +
            kUserColumns,
        name, email, id);
    if (result.empty()) {
        throw std::runtime_error("Usuario no encontrado");
    }
    co_return fromRow(result[0]);
}

drogon::Task<core::User> UserRepository::setPasswordHash(
    int64_t id, const std::string& passwordHash) const {
    auto result = co_await db_->execSqlCoro(
        std::string("UPDATE users SET password_hash = $1 WHERE id = $2 "
                    "RETURNING ") +
            kUserColumns,
        passwordHash, id);
    if (result.empty()) {
        throw std::runtime_error("Usuario no encontrado");
    }
    co_return fromRow(result[0]);
}

drogon::Task<core::User> UserRepository::setGoal(int64_t id,
                                                 double goalWeightKg) const {
    auto result = co_await db_->execSqlCoro(
        std::string("UPDATE users SET goal_weight_kg = $1 WHERE id = $2 "
                    "RETURNING ") +
            kUserColumns,
        goalWeightKg, id);
    if (result.empty()) {
        throw std::runtime_error("Usuario no encontrado");
    }
    co_return fromRow(result[0]);
}

}  // namespace healthiq::database
