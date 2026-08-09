#include "database/SeedData.h"

#include "ai/ModelState.h"
#include "ai/Predictor.h"
#include "core/BmiCalculator.h"
#include "security/PasswordHasher.h"

#include <chrono>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace healthiq::database {

namespace {

const char* kDemoEmail = "demo@healthiq.app";
const char* kDemoName = "Usuario Demo";
const char* kDemoPassword = "demo1234";
const double kDemoHeightCm = 170.0;
const int kDemoActivity = 3;

struct DemoPoint {
    double weightKg;
    int daysAgo;
    const char* note;
};

// Historial de 12 mediciones repartidas en ~11 semanas con tendencia
// descendente, suficiente para que el modelo entrene con confianza "alta".
const std::vector<DemoPoint>& demoPoints() {
    static const std::vector<DemoPoint> points = {
        {85.0, 77, "inicio"},
        {84.4, 70, ""},
        {83.7, 63, ""},
        {83.0, 56, "rutina de fuerza"},
        {82.2, 49, ""},
        {81.5, 42, ""},
        {80.8, 35, ""},
        {80.0, 28, "cardio 3x semana"},
        {79.4, 21, ""},
        {78.6, 14, ""},
        {78.0, 7, ""},
        {77.3, 0, "medicion actual"},
    };
    return points;
}

int64_t epochSecondsDaysAgo(int daysAgo) {
    const auto now = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::seconds>(
               (now - std::chrono::hours(24 * daysAgo)).time_since_epoch())
        .count();
}

int64_t epochDays(int64_t epochSeconds) {
    return epochSeconds / (24LL * 3600LL);
}

// Formatea epoch seconds como "YYYY-MM-DD HH:MM:SS" (UTC). Se pasa como texto
// y se castea en SQL porque to_timestamp(int8) no resuelve en sentencias
// preparadas con el parametro tipado como bigint.
std::string formatIsoUtc(int64_t epochSeconds) {
    const std::time_t t = static_cast<std::time_t>(epochSeconds);
    std::tm tmv{};
#ifdef _WIN32
    gmtime_s(&tmv, &t);
#else
    gmtime_r(&t, &tmv);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tmv, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

}  // namespace

void SeedData::run(drogon::orm::DbClientPtr db) {
    try {
        auto existing =
            db->execSqlSync("SELECT id FROM users WHERE email = $1 LIMIT 1",
                            kDemoEmail);
        if (!existing.empty()) {
            LOG_INFO << "Seed: usuario demo ya existe, omitiendo.";
            return;
        }

        const auto hash = security::PasswordHasher::hash(kDemoPassword);
        auto ures = db->execSqlSync(
            "INSERT INTO users(email, name, password_hash, role) "
            "VALUES($1, $2, $3, 'user') RETURNING id",
            kDemoEmail, kDemoName, hash);
        const int64_t userId = ures[0]["id"].as<int64_t>();

        // Insertar historial y entrenar el modelo en orden cronologico.
        ai::ModelState model;
        model.userId = userId;
        const auto& points = demoPoints();
        for (const auto& p : points) {
            const auto bmi =
                core::BmiCalculator::calculate(p.weightKg, kDemoHeightCm);
            const int64_t createdAt = epochSecondsDaysAgo(p.daysAgo);
            db->execSqlSync(
                "INSERT INTO health_records(user_id, weight_kg, height_cm, "
                "bmi, activity_level, note, created_at) "
                "VALUES($1, $2, $3, $4, $5, $6, $7::timestamptz)",
                userId, p.weightKg, kDemoHeightCm, bmi.bmi, kDemoActivity,
                p.note, formatIsoUtc(createdAt));

            model = ai::Predictor::learn(std::move(model), p.weightKg,
                                         kDemoHeightCm, epochDays(createdAt));
            model.lastBmi = bmi.bmi;
        }

        db->execSqlSync(
            "INSERT INTO ai_models(user_id, slope, intercept, sample_count, "
            "last_weight, last_height, last_bmi, t_start, sum_x, sum_y, "
            "sum_xx, sum_xy, last_updated) "
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
            userId, model.slope, model.intercept, model.sampleCount,
            model.lastWeight, model.lastHeight, model.lastBmi, model.tStart,
            model.sumX, model.sumY, model.sumXX, model.sumXY);

        LOG_INFO << "Seed: usuario demo creado (" << kDemoEmail << ") con "
                 << model.sampleCount
                 << " mediciones y modelo de IA entrenado.";
    } catch (const std::exception& e) {
        LOG_ERROR << "Seed: no se pudo cargar datos demo: " << e.what();
    }
}

}  // namespace healthiq::database
