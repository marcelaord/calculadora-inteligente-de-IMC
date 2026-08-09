#include "ai/Predictor.h"

#include <cmath>

namespace healthiq::ai {

ModelState Predictor::learn(ModelState model,
                            double weightKg,
                            double heightCm,
                            int64_t nowEpochDays) {
    // Regresion lineal exacta en linea (minimos cuadrados recursivos).
    // t se mide como dias desde tStart (x=0,1,2,...) para estabilidad numerica.
    if (model.sampleCount == 0 || model.tStart == 0) {
        model.tStart = nowEpochDays;
        model.sampleCount = 1;
        model.sumX = 0.0;
        model.sumY = weightKg;
        model.sumXX = 0.0;
        model.sumXY = 0.0;
        model.slope = 0.0;
        model.intercept = weightKg;
        model.lastWeight = weightKg;
        model.lastHeight = heightCm;
        model.lastBmi = 0.0;
        return model;
    }

    const double x = static_cast<double>(nowEpochDays - model.tStart);
    const double y = weightKg;

    model.sumX += x;
    model.sumY += y;
    model.sumXX += x * x;
    model.sumXY += x * y;
    model.sampleCount += 1;

    const double n = static_cast<double>(model.sampleCount);
    const double denom = n * model.sumXX - model.sumX * model.sumX;
    if (std::abs(denom) > 1e-9) {
        model.slope = (n * model.sumXY - model.sumX * model.sumY) / denom;
        model.intercept = (model.sumY - model.slope * model.sumX) / n;
    }

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
