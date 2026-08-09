#include "ai/Predictor.h"

#include <cmath>

namespace healthiq::ai {

namespace {
// Minimos cuadrados incrementales (recursive least squares).
// Mantenemos sumas acumuladas: Sxx, Sxy, Sx, Sy, n.
struct Accum {
    double sumX = 0.0;
    double sumY = 0.0;
    double sumXX = 0.0;
    double sumXY = 0.0;
    double n = 0.0;
};

Accum accumFromModel(const ModelState& m) {
    // Reconstruimos las sumas a partir de slope/intercept y el primer punto.
    // Para 1 punto la recta pasa por el punto: slope=0.
    // Para n puntos usamos las formulas cerradas asumiendo x centrado en tStart.
    // t se mide como dias desde tStart (x=0,1,2,...) para estabilidad numerica.
    Accum a;
    a.n = static_cast<double>(m.sampleCount);
    return a;
}
}  // namespace

ModelState Predictor::learn(ModelState model,
                            double weightKg,
                            double heightCm,
                            int64_t nowEpochDays) {
    // Convertimos a dias relativos al modelo para estabilidad numerica.
    if (model.sampleCount == 0 || model.tStart == 0) {
        model.tStart = nowEpochDays;
        model.sampleCount = 1;
        model.intercept = weightKg;
        model.slope = 0.0;
        model.lastWeight = weightKg;
        model.lastHeight = heightCm;
        model.lastBmi = 0.0;
        return model;
    }

    const double x = static_cast<double>(nowEpochDays - model.tStart);
    const double y = weightKg;

    // Estimacion actual del modelo.
    const double yEst = model.intercept + model.slope * x;
    const double error = y - yEst;

    // Actualizacion con gradiente descendente estocastico (SGD) en linea.
    const double lr = 1.0 / (static_cast<double>(model.sampleCount) + 1.0);
    model.intercept += lr * error;
    model.slope += lr * error * x;

    model.sampleCount += 1;
    model.lastWeight = weightKg;
    model.lastHeight = heightCm;
    return model;
}

Prediction Predictor::predict(const ModelState& model,
                              double currentWeight,
                              double heightCm,
                              int64_t nowEpochDays) {
    Prediction p;
    p.slope = model.slope;
    p.sampleCount = model.sampleCount;
    p.currentWeight = currentWeight;

    const double daysFromStart = static_cast<double>(nowEpochDays - model.tStart);
    const double weightNow = model.predictWeightAt(static_cast<int64_t>(daysFromStart));

    p.weightIn7d = weightNow + model.slope * 7.0;
    p.weightIn30d = weightNow + model.slope * 30.0;
    p.weightIn90d = weightNow + model.slope * 90.0;
    p.trendKgPerMonth = model.slope * 30.0;

    const double hM = heightCm / 100.0;
    const double h2 = hM * hM;
    p.bmiIn30d = (h2 > 0) ? p.weightIn30d / h2 : 0.0;
    p.bmiIn90d = (h2 > 0) ? p.weightIn90d / h2 : 0.0;

    if (model.sampleCount >= 10) {
        p.confidence = "alta";
    } else if (model.sampleCount >= 4) {
        p.confidence = "media";
    } else {
        p.confidence = "baja";
    }
    return p;
}

}  // namespace healthiq::ai
