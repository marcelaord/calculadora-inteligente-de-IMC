#ifndef HEALTHIQ_AI_PREDICTOR_H
#define HEALTHIQ_AI_PREDICTOR_H

#include "ai/ModelState.h"

namespace healthiq::ai {

// Actualiza el modelo de regresion lineal en linea (aprendizaje continuo)
// usando la tecnica de minimos cuadrados incrementales.
class Predictor {
public:
    static ModelState learn(ModelState model,
                            double weightKg,
                            double heightCm,
                            int64_t nowEpochDays);

    static Prediction predict(const ModelState& model,
                              double currentWeight,
                              double heightCm,
                              int64_t nowEpochDays);
};

}  // namespace healthiq::ai

#endif
