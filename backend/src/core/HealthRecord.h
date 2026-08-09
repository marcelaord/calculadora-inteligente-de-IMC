#ifndef HEALTHIQ_CORE_HEALTH_RECORD_H
#define HEALTHIQ_CORE_HEALTH_RECORD_H

#include <cstdint>
#include <string>

namespace healthiq::core {

struct HealthRecord {
    int64_t id{0};
    int64_t userId{0};
    double weightKg{0.0};
    double heightCm{0.0};
    double bmi{0.0};
    int activityLevel{3};  // 1..5
    std::string note;
    std::string createdAt;
};

struct User {
    int64_t id{0};
    std::string email;
    std::string name;
    std::string passwordHash;
    std::string role{"user"};
    std::string createdAt;
};

}  // namespace healthiq::core

#endif
