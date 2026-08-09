cmake_minimum_required(VERSION 3.20)

project(healthiq-backend VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Drogon REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

add_executable(healthiq-backend
    src/main.cpp

    src/AppServices.cpp
    src/AppServices.h

    src/config/AppConfig.cpp
    src/config/AppConfig.h

    src/core/BmiCalculator.cpp
    src/core/BmiCalculator.h
    src/core/Enums.h
    src/core/HealthRecord.h

    src/database/Db.cpp
    src/database/Db.h
    src/database/SeedData.cpp
    src/database/SeedData.h
    src/database/UserRepository.cpp
    src/database/UserRepository.h
    src/database/HealthRecordRepository.cpp
    src/database/HealthRecordRepository.h
    src/database/AiModelRepository.cpp
    src/database/AiModelRepository.h

    src/security/JwtService.cpp
    src/security/JwtService.h
    src/security/PasswordHasher.cpp
    src/security/PasswordHasher.h

    src/ai/ModelState.h
    src/ai/Predictor.cpp
    src/ai/Predictor.h
    src/ai/Recommender.cpp
    src/ai/Recommender.h
    src/ai/AiEngine.cpp
    src/ai/AiEngine.h

    src/api/HttpResponses.h
    src/api/middleware/JwtAuthFilter.cpp
    src/api/middleware/JwtAuthFilter.h
    src/api/controllers/AuthController.cpp
    src/api/controllers/AuthController.h
    src/api/controllers/HealthController.cpp
    src/api/controllers/HealthController.h
    src/api/controllers/AiController.cpp
    src/api/controllers/AiController.h
    src/api/controllers/DashboardController.cpp
    src/api/controllers/DashboardController.h
    src/api/websocket/DashboardWs.cpp
    src/api/websocket/DashboardWs.h
)

target_link_libraries(healthiq-backend
    PRIVATE
        Drogon::Drogon
        OpenSSL::SSL
        OpenSSL::Crypto
        Threads::Threads
)

target_include_directories(healthiq-backend PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

set_target_properties(healthiq-backend PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
