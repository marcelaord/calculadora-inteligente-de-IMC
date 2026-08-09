#ifndef HEALTHIQ_CORE_BMI_CALCULATOR_H
#define HEALTHIQ_CORE_BMI_CALCULATOR_H

#include "core/Enums.h"

namespace healthiq::core {

struct BmiResult {
    double bmi{0.0};
    double heightMeters{0.0};
    double heightSquared{0.0};
    BmiCategory category{BmiCategory::Normal};
};

class BmiCalculator {
public:
    // Formula tradicional: IMC = peso(kg) / (altura(m))^2
    static BmiResult calculate(double weightKg, double heightCm);
    static BmiCategory classify(double bmi);
};

}  // namespace healthiq::core

#endif
