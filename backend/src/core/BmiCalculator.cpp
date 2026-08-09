#include "core/BmiCalculator.h"

#include <cmath>

namespace healthiq::core {

BmiResult BmiCalculator::calculate(double weightKg, double heightCm) {
    BmiResult r;
    r.heightMeters = heightCm / 100.0;
    r.heightSquared = r.heightMeters * r.heightMeters;
    r.bmi = (r.heightSquared > 0.0) ? weightKg / r.heightSquared : 0.0;
    r.category = classify(r.bmi);
    return r;
}

BmiCategory BmiCalculator::classify(double bmi) {
    if (bmi < 18.5) return BmiCategory::Underweight;
    if (bmi < 25.0) return BmiCategory::Normal;
    if (bmi < 30.0) return BmiCategory::Overweight;
    if (bmi < 35.0) return BmiCategory::ObesityI;
    if (bmi < 40.0) return BmiCategory::ObesityII;
    return BmiCategory::ObesityIII;
}

}  // namespace healthiq::core
