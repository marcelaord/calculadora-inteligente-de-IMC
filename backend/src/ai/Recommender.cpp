#include "ai/Recommender.h"

#include <cmath>

namespace healthiq::ai {

std::vector<Recommendation> Recommender::recommend(const Prediction& pred,
                                                   double bmi,
                                                   int activityLevel) {
    std::vector<Recommendation> out;

    auto add = [&](std::string title, std::string detail, std::string priority,
                   std::string category) {
        out.push_back({std::move(title), std::move(detail), std::move(priority),
                       std::move(category)});
    };

    if (bmi < 18.5) {
        add("Aumento de aporte calorico",
            "Tu IMC indica bajo peso. Prioriza alimentos densos en nutrientes: "
            "proteinas, cereales integrales, grasas saludables y 4-5 comidas al dia.",
            "alta", "nutricion");
    } else if (bmi < 25.0) {
        add("Mantener habitos saludables",
            "Tu IMC esta en el rango normal. Manten una dieta balanceada, "
            "hidratacion adecuada y descanso de 7-8 horas.",
            "baja", "salud");
    } else if (bmi < 30.0) {
        add("Deficit calorico moderado",
            "Estas en sobrepeso. Reduce ~500 kcal/dia y prioriza vegetales, "
            "proteina magra y fibra.",
            "alta", "nutricion");
    } else if (bmi < 35.0) {
        add("Plan de reduccion supervisado",
            "Obesidad grado I. Se recomienda un deficit de 500-750 kcal/dia, "
            "acompanado de seguimiento profesional.",
            "alta", "nutricion");
    } else {
        add("Atencion medica especializada",
            "Obesidad grado II o III. Es importante acudir con un especialista "
            "antes de iniciar cualquier plan.",
            "alta", "salud");
    }

    if (pred.slope > 0.05) {
        add("Tendencia de aumento detectada",
            "El modelo detecta un aumento de aproximadamente " +
                std::to_string(pred.trendKgPerMonth) +
                " kg/mes. Reduce porciones y aumenta la actividad fisica.",
            "alta", "actividad");
    } else if (pred.slope < -0.05) {
        add("Perdida sostenida detectada",
            "El modelo detecta una perdida de aproximadamente " +
                std::to_string(-pred.trendKgPerMonth) +
                " kg/mes. Ideal entre 0.5-1 kg/semana; evita perdidas muy rapidas.",
            "media", "actividad");
    } else {
        add("Peso estable",
            "Tu peso se mantiene estable. Para mejorar la composicion corporal, "
            "anade entrenamiento de fuerza 2-3 veces por semana.",
            "baja", "actividad");
    }

    if (activityLevel <= 2) {
        add("Incrementar actividad fisica",
            "Tu nivel de actividad es bajo. Empieza con 30 minutos de caminata "
            "diaria y aumenta progresivamente.",
            "media", "actividad");
    } else if (activityLevel >= 4) {
        add("Mantener actividad intensa",
            "Tu nivel de actividad es alto. Asegurate de cubrir el aporte "
            "proteico y la recuperacion.",
            "baja", "actividad");
    }

    if (pred.sampleCount < 4) {
        add("Registra mas datos",
            "El modelo necesita al menos 4 registros para hacer predicciones "
            "confiables. Registra tu peso cada 2-3 dias.",
            "media", "salud");
    }

    if (bmi >= 25.0) {
        add("Meta semanal recomendada",
            "Objetivo sugerido: perder 0.5-1 kg por semana. En 30 dias podrias "
            "alcanzar " + std::to_string(pred.bmiIn30d) + " de IMC.",
            "media", "salud");
    }

    return out;
}

}  // namespace healthiq::ai
