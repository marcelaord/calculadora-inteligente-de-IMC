#ifndef HEALTHIQ_AI_MODEL_STATE_H
#define HEALTHIQ_AI_MODEL_STATE_H

#include <cstdint>
#include <string>
#include <vector>

namespace healthiq::ai {

// Estado persistente del modelo de aprendizaje por usuario.
// El modelo es una regresion lineal en linea (online linear regression):
//   peso(t) = intercept + slope * t
// donde t es el numero de dias transcurridos desde tStart.
struct ModelState {
    int64_t userId{0};
    double slope{0.0};      // kg/dia
    double intercept{0.0};  // kg
    int sampleCount{0};     // numero de muestras aprendidas
    double lastWeight{0.0};
    double lastHeight{0.0};
    double lastBmi{0.0};
    int64_t tStart{0};  // epoch en dias

    // Sumas acumuladas para minimos cuadrados recursivos exactos:
    // slope = (n*Sxy - Sx*Sy) / (n*Sxx - Sx^2), intercept = (Sy - slope*Sx)/n
    double sumX{0.0};
    double sumY{0.0};
    double sumXX{0.0};
    double sumXY{0.0};

    double predictWeightAt(int64_t daysFromStart) const {
        return intercept + slope * static_cast<double>(daysFromStart);
    }
};

struct Prediction {
    double currentWeight{0.0};
    double weightIn7d{0.0};
    double weightIn30d{0.0};
    double weightIn90d{0.0};
    double bmiIn30d{0.0};
    double bmiIn90d{0.0};
    double trendKgPerMonth{0.0};
    double slope{0.0};
    int sampleCount{0};
    std::string confidence;  // alta/media/baja segun muestras
};

}  // namespace healthiq::ai

#endif
