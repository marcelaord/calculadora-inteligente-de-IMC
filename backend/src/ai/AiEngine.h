#ifndef HEALTHIQ_AI_AI_ENGINE_H
#define HEALTHIQ_AI_AI_ENGINE_H

#include "ai/ModelState.h"
#include "database/AiModelRepository.h"
#include "database/HealthRecordRepository.h"

#include <memory>

namespace healthiq::ai {

// Orquesta el motor de inteligencia: aprende de cada registro, predice
// tendencias y genera recomendaciones personalizadas.
class AiEngine {
public:
    AiEngine(database::AiModelRepository models, database::HealthRecordRepository records);

    // Aprende del nuevo registro y persiste el modelo actualizado.
    drogon::Task<ModelState> learn(int64_t userId, double weightKg,
                                   double heightCm);

    // Genera predicciones y recomendaciones para el usuario.
    drogon::Task<Json::Value> analyze(int64_t userId);

    // Resumen para el dashboard en tiempo real.
    drogon::Task<Json::Value> dashboard(int64_t userId);

private:
    static int64_t nowEpochDays();
    database::AiModelRepository models_;
    database::HealthRecordRepository records_;
};

}  // namespace healthiq::ai

#endif
