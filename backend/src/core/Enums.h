#ifndef HEALTHIQ_CORE_ENUMS_H
#define HEALTHIQ_CORE_ENUMS_H

#include <string>

namespace healthiq::core {

enum class BmiCategory {
    Underweight,
    Normal,
    Overweight,
    ObesityI,
    ObesityII,
    ObesityIII
};

inline std::string categoryToString(BmiCategory c) {
    switch (c) {
        case BmiCategory::Underweight: return "Bajo peso";
        case BmiCategory::Normal:      return "Peso normal";
        case BmiCategory::Overweight:  return "Sobrepeso";
        case BmiCategory::ObesityI:    return "Obesidad grado I";
        case BmiCategory::ObesityII:   return "Obesidad grado II";
        case BmiCategory::ObesityIII:  return "Obesidad grado III";
    }
    return "Desconocido";
}

}  // namespace healthiq::core

#endif
