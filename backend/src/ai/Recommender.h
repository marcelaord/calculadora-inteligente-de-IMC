#ifndef HEALTHIQ_AI_RECOMMENDER_H
#define HEALTHIQ_AI_RECOMMENDER_H

#include "ai/ModelState.h"

#include <string>
#include <vector>

namespace healthiq::ai {

struct Recommendation {
    std::string title;
    std::string detail;
    std::string priority;  // alta/media/baja
    std::string category;  // nutricion/actividad/salud
};

class Recommender {
public:
    static std::vector<Recommendation> recommend(const Prediction& pred,
                                                 double bmi,
                                                 int activityLevel);
};

}  // namespace healthiq::ai

#endif
