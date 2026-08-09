#include "database/UserRepository.h"

#include <drogon/orm/Exception.h>

#include <exception>
#include <stdexcept>

namespace healthiq::database {

UserRepository::UserRepository(drogon::orm::DbClientPtr db) : db_(std::move(db)) {}

core::User UserRepository::fromRow(const drogon::orm::Row& row) {
    core::User u;
    u.id = row["id"].as<int64_t>();
    u.email = row["email"].as<std::string>();
    u.name = row["name"].as<std::string>();
    u.passwordHash = row["password_hash"].as<std::string>();
    u.role = row["role"].as<std::string>();
    u.createdAt = row["created_at"].as<std::string>();
    return u;
}

drogon::Task<core::User> UserRepository::create(const std::string& email,
                                                const std::string& name,
                                                const std::string& passwordHash) const {
    auto result = co_await db_->execSqlCoro(
        "INSERT INTO users(email, name, password_hash) VALUES($1, $2, $3) "
        "RETURNING id, email, name, password_hash, role, created_at",
        email, name, passwordHash);
    if (result.empty()) {
        throw std::runtime_error("No se pudo crear el usuario");
    }
    co_return fromRow(result[0]);
}

drogon::Task<std::optional<core::User>> UserRepository::findByEmail(
    const std::string& email) const {
    auto result = co_await db_->execSqlCoro(
        "SELECT id, email, name, password_hash, role, created_at "
        "FROM users WHERE email = $1 LIMIT 1",
        email);
    if (result.empty()) {
        co_return std::nullopt;
    }
    co_return fromRow(result[0]);
}

drogon::Task<std::optional<core::User>> UserRepository::findById(int64_t id) const {
    auto result = co_await db_->execSqlCoro(
        "SELECT id, email, name, password_hash, role, created_at "
        "FROM users WHERE id = $1 LIMIT 1",
        id);
    if (result.empty()) {
        co_return std::nullopt;
    }
    co_return fromRow(result[0]);
}

}  // namespace healthiq::database
