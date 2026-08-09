#include "database/Db.h"

#include <drogon/HttpAppFramework.h>

namespace healthiq::database {

void Db::init(const std::string& connInfo, size_t poolSize) {
    client_ = drogon::orm::DbClient::newPgClient(connInfo, poolSize);
    LOG_INFO << "Pool de PostgreSQL conectado (host definido en PGHOST/PGPORT).";
}

void Db::createSchema() {
    if (!client_) {
        LOG_ERROR << "No se pudo inicializar el esquema: pool no disponible.";
        return;
    }
    // PostgreSQL no admite multiples comandos en una sentencia preparada,
    // asi que cada sentencia del esquema se ejecuta por separado.
    const std::vector<std::string> statements = {
        R"SQL(CREATE TABLE IF NOT EXISTS users (
    id            BIGSERIAL PRIMARY KEY,
    email         TEXT UNIQUE NOT NULL,
    name          TEXT NOT NULL,
    password_hash TEXT NOT NULL,
    role          TEXT NOT NULL DEFAULT 'user',
    goal_weight_kg DOUBLE PRECISION NOT NULL DEFAULT 0,
    created_at    TIMESTAMPTZ NOT NULL DEFAULT now()
))SQL",
        R"SQL(CREATE TABLE IF NOT EXISTS health_records (
    id            BIGSERIAL PRIMARY KEY,
    user_id       BIGINT NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    weight_kg     NUMERIC(6,2) NOT NULL,
    height_cm     NUMERIC(6,2) NOT NULL,
    bmi           NUMERIC(5,2) NOT NULL,
    activity_level INT NOT NULL DEFAULT 3,
    note          TEXT NOT NULL DEFAULT '',
    created_at    TIMESTAMPTZ NOT NULL DEFAULT now()
))SQL",
        R"SQL(CREATE INDEX IF NOT EXISTS idx_health_records_user_created
    ON health_records(user_id, created_at))SQL",
        R"SQL(CREATE TABLE IF NOT EXISTS ai_models (
    user_id       BIGINT PRIMARY KEY REFERENCES users(id) ON DELETE CASCADE,
    slope         DOUBLE PRECISION NOT NULL DEFAULT 0,
    intercept     DOUBLE PRECISION NOT NULL DEFAULT 0,
    sample_count  INT NOT NULL DEFAULT 0,
    last_weight   DOUBLE PRECISION NOT NULL DEFAULT 0,
    last_height   DOUBLE PRECISION NOT NULL DEFAULT 0,
    last_bmi      DOUBLE PRECISION NOT NULL DEFAULT 0,
    t_start       BIGINT NOT NULL DEFAULT 0,
    sum_x         DOUBLE PRECISION NOT NULL DEFAULT 0,
    sum_y         DOUBLE PRECISION NOT NULL DEFAULT 0,
    sum_xx        DOUBLE PRECISION NOT NULL DEFAULT 0,
    sum_xy        DOUBLE PRECISION NOT NULL DEFAULT 0,
    last_updated  TIMESTAMPTZ NOT NULL DEFAULT now()
))SQL",
        // Migracion para bases existentes (idempotente).
        R"SQL(ALTER TABLE ai_models
    ADD COLUMN IF NOT EXISTS sum_x DOUBLE PRECISION NOT NULL DEFAULT 0)SQL",
        R"SQL(ALTER TABLE ai_models
    ADD COLUMN IF NOT EXISTS sum_y DOUBLE PRECISION NOT NULL DEFAULT 0)SQL",
        R"SQL(ALTER TABLE ai_models
    ADD COLUMN IF NOT EXISTS sum_xx DOUBLE PRECISION NOT NULL DEFAULT 0)SQL",
        R"SQL(ALTER TABLE ai_models
    ADD COLUMN IF NOT EXISTS sum_xy DOUBLE PRECISION NOT NULL DEFAULT 0)SQL",
        // Migracion para bases existentes (idempotente).
        R"SQL(ALTER TABLE users
    ADD COLUMN IF NOT EXISTS goal_weight_kg DOUBLE PRECISION NOT NULL DEFAULT 0)SQL",
    };

    try {
        // Ejecucion sincrona (una sola vez al arrancar).
        for (const auto& stmt : statements) {
            client_->execSqlSync(stmt);
        }
        LOG_INFO << "Esquema de base de datos verificado/creado.";
    } catch (const std::exception& e) {
        LOG_ERROR << "Error al crear el esquema: " << e.what();
    }
}

drogon::orm::DbClientPtr Db::client() {
    return client_;
}

}  // namespace healthiq::database
