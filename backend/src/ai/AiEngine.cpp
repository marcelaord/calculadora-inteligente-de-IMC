#include "ai/AiEngine.h"

#include <drogon/utils/Utilities.h>

#include "ai/Predictor.h"
#include "ai/Recommender.h"
#include "core/BmiCalculator.h"

#include <chrono>
#include <cmath>

namespace healthiq::ai {

AiEngine::AiEngine(database::AiModelRepository models,
                   database::HealthRecordRepository records)
    : models_(std::move(models)), records_(std::move(records)) {}

int64_t AiEngine::nowEpochDays() {
    return static_cast<int64_t>(
        std::chrono::system_clock::now().time_since_epoch() /
        std::chrono::days(1));
}

drogon::Task<ModelState> AiEngine::learn(int64_t userId, double weightKg,
                                         double heightCm) {
    auto model = co_await models_.loadOrCreate(userId, weightKg, heightCm);
    model = Predictor::learn(std::move(model), weightKg, heightCm, nowEpochDays());
    const auto bmi = core::BmiCalculator::calculate(weightKg, heightCm);
    model.lastBmi = bmi.bmi;
    co_await models_.save(model);
    co_return model;
}

drogon::Task<Json::Value> AiEngine::analyze(int64_t userId) {
    auto model = co_await models_.load(userId);
    auto latest = co_await records_.latest(userId);

    Json::Value out(Json::objectValue);

    if (!model || !latest) {
        out["available"] = false;
        out["message"] = "Aun no hay suficientes datos. Registra tu primer peso.";
        co_return out;
    }

    const auto pred =
        Predictor::predict(*model, latest->weightKg, latest->heightCm, nowEpochDays());
    const auto recs = Recommender::recommend(pred, latest->bmi, latest->activityLevel);

    out["available"] = true;
    out["bmi"] = latest->bmi;
    out["category"] = core::categoryToString(
        core::BmiCalculator::classify(latest->bmi));
    out["currentWeight"] = latest->weightKg;
    out["prediction"]["weight7d"] = pred.weightIn7d;
    out["prediction"]["weight30d"] = pred.weightIn30d;
    out["prediction"]["weight90d"] = pred.weightIn90d;
    out["prediction"]["bmi30d"] = pred.bmiIn30d;
    out["prediction"]["bmi90d"] = pred.bmiIn90d;
    out["prediction"]["trendKgPerMonth"] = pred.trendKgPerMonth;
    out["prediction"]["slope"] = pred.slope;
    out["prediction"]["confidence"] = pred.confidence;
    out["prediction"]["sampleCount"] = model->sampleCount;
    out["prediction"]["modelType"] = "Regresion lineal (minimos cuadrados)";

    // Clasificacion de tendencia y objetivo saludable.
    {
        const double slope = pred.slope;
        std::string trend = "estable";
        if (std::abs(slope) >= 0.01) {
            trend = (slope < 0) ? "descendente" : "ascendente";
        }

        const auto cat = core::BmiCalculator::classify(latest->bmi);
        std::string signal;
        if (cat == core::BmiCategory::Normal) {
            signal = (std::abs(slope) < 0.01) ? "favorable"
                                              : "estable";
        } else if (cat == core::BmiCategory::Underweight) {
            signal = (slope > 0.01)   ? "favorable"
                     : (slope < -0.01) ? "desfavorable"
                                       : "estable";
        } else {  // Sobrepeso / Obesidad
            signal = (slope < -0.01)  ? "favorable"
                     : (slope > 0.01) ? "desfavorable"
                                      : "estable";
        }

        // Limite del rango saludable (IMC 18.5 - 24.9) al que apuntar.
        double healthyTargetKg = 0.0;
        if (latest->heightCm > 0.0) {
            const double h2 = std::pow(latest->heightCm / 100.0, 2.0);
            healthyTargetKg =
                (cat == core::BmiCategory::Underweight) ? 18.5 * h2 : 24.9 * h2;
        }

        double daysToHealthy = -1.0;
        if (healthyTargetKg > 0.0 && std::abs(slope) > 1e-9) {
            const double delta = latest->weightKg - healthyTargetKg;
            const bool converges =
                (delta > 0.0 && slope < 0.0) || (delta < 0.0 && slope > 0.0);
            if (converges) {
                daysToHealthy = std::abs(delta / slope);
            }
        }

        char buf[192];
        std::string insight;
        if (std::abs(slope) < 0.01) {
            std::snprintf(buf, sizeof(buf),
                          "Tu IMC (%.1f) se ha mantenido estable en los ultimos "
                          "registros. El modelo estima que la tendencia se mantendra "
                          "en el corto plazo.",
                          latest->bmi);
            insight = buf;
        } else if (trend == "descendente") {
            if (daysToHealthy >= 0.0 && daysToHealthy <= 730.0) {
                std::snprintf(buf, sizeof(buf),
                              "Tu IMC (%.1f) presenta una tendencia descendente. "
                              "Si el comportamiento observado se mantiene, el modelo "
                              "estima que podrias acercarte al rango saludable en "
                              "unos %.0f dias (~%d meses).",
                              latest->bmi, daysToHealthy,
                              static_cast<int>(std::round(daysToHealthy / 30.0)));
            } else {
                std::snprintf(buf, sizeof(buf),
                              "Tu IMC (%.1f) presenta una tendencia descendente en "
                              "los ultimos registros. Si el comportamiento se "
                              "mantiene, el modelo estima que el descenso continuara.",
                              latest->bmi);
            }
            insight = buf;
        } else {
            std::snprintf(buf, sizeof(buf),
                          "Tu IMC (%.1f) presenta una tendencia ascendente en los "
                          "ultimos registros. Conviene revisar tus habitos para "
                          "evitar que el aumento se consolide.",
                          latest->bmi);
            insight = buf;
        }

        out["trend"] = trend;
        out["signal"] = signal;
        out["insight"] = insight;
        out["prediction"]["healthyTargetKg"] = healthyTargetKg;
        out["prediction"]["daysToHealthy"] =
            (daysToHealthy >= 0.0) ? static_cast<int>(std::round(daysToHealthy))
                                   : -1;
    }

    // Metricas de precision sobre todo el historial: R^2, RMSE y MAE.
    {
        const auto chrono = co_await records_.listChronological(userId);
        double ssRes = 0.0;
        double ssTot = 0.0;
        double maeSum = 0.0;
        double meanW = 0.0;
        const double n = static_cast<double>(chrono.size());
        if (n > 0.0) {
            for (const auto& r : chrono) {
                meanW += r.weightKg;
            }
            meanW /= n;
            for (const auto& r : chrono) {
                const double predW = model->predictWeightAt(r.epochDay - model->tStart);
                const double res = r.weightKg - predW;
                ssRes += res * res;
                maeSum += std::abs(res);
                const double d = r.weightKg - meanW;
                ssTot += d * d;
            }
            const double r2 = (ssTot > 1e-12) ? 1.0 - ssRes / ssTot : 0.0;
            const double rmse = std::sqrt(ssRes / n);
            out["prediction"]["r2"] = r2;
            out["prediction"]["rmse"] = rmse;
            out["prediction"]["mae"] = maeSum / n;
        } else {
            out["prediction"]["r2"] = 0.0;
            out["prediction"]["rmse"] = 0.0;
            out["prediction"]["mae"] = 0.0;
        }
    }

    Json::Value recArray(Json::arrayValue);
    for (const auto& r : recs) {
        Json::Value item(Json::objectValue);
        item["title"] = r.title;
        item["detail"] = r.detail;
        item["priority"] = r.priority;
        item["category"] = r.category;
        recArray.append(item);
    }
    out["recommendations"] = recArray;
    co_return out;
}

drogon::Task<Json::Value> AiEngine::dashboard(int64_t userId) {
    auto latest = co_await records_.latest(userId);
    auto records = co_await records_.listByUser(userId, 90);
    auto analysis = co_await analyze(userId);

    Json::Value out(Json::objectValue);
    out["hasData"] = !records.empty();
    out["user"] = userId;

    if (!records.empty()) {
        double sum = 0.0;
        for (const auto& r : records) {
            sum += r.weightKg;
        }
        out["stats"]["recordCount"] = static_cast<int>(records.size());
        out["stats"]["avgWeight"] = sum / static_cast<double>(records.size());
        out["stats"]["latestBmi"] = latest ? latest->bmi : 0.0;
        out["stats"]["latestWeight"] = latest ? latest->weightKg : 0.0;
    } else {
        out["stats"]["recordCount"] = 0;
    }

    Json::Value history(Json::arrayValue);
    for (auto it = records.rbegin(); it != records.rend(); ++it) {
        Json::Value item(Json::objectValue);
        item["date"] = it->createdAt;
        item["weightKg"] = it->weightKg;
        item["bmi"] = it->bmi;
        history.append(item);
    }
    out["history"] = history;
    out["analysis"] = analysis;
    co_return out;
}

drogon::Task<void> AiEngine::rebuild(int64_t userId) {
    auto records = co_await records_.listChronological(userId);
    ai::ModelState model;
    model.userId = userId;
    for (const auto& r : records) {
        model = Predictor::learn(std::move(model), r.weightKg, r.heightCm, r.epochDay);
        model.lastBmi = r.bmi;
    }
    co_await models_.save(model);
    co_return;
}

}  // namespace healthiq::ai
