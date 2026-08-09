#ifndef HEALTHIQ_DATABASE_SEED_DATA_H
#define HEALTHIQ_DATABASE_SEED_DATA_H

#include <drogon/orm/DbClient.h>

namespace healthiq::database {

// Carga datos de demostracion la primera vez que arranca el backend:
// un usuario demo con historial de mediciones y su modelo de IA entrenado.
class SeedData {
public:
    static void run(drogon::orm::DbClientPtr db);
};

}  // namespace healthiq::database

#endif
