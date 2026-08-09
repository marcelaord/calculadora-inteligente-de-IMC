#include "api/controllers/HealthController.h"

#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>

#include "AppServices.h"
#include "ai/AiEngine.h"
#include "api/HttpResponses.h"
#include "api/websocket/DashboardWs.h"
#include "core/BmiCalculator.h"

#include <sstream>
#include <stdexcept>

namespace healthiq::api {
namespace {

Json::Value recordToJson(const core::HealthRecord& r) {
    Json::Value item(Json::objectValue);
    item["id"] = r.id;
    item["weightKg"] = r.weightKg;
    item["heightCm"] = r.heightCm;
    item["bmi"] = r.bmi;
    item["activityLevel"] = r.activityLevel;
    item["note"] = r.note;
    item["createdAt"] = r.createdAt;
    return item;
}

std::string csvEscape(const std::string& value) {
    if (value.find_first_of(",\"\n\r") == std::string::npos) {
        return value;
    }
    std::string escaped;
    escaped.reserve(value.size() + 2);
    escaped.push_back('"');
    for (const char c : value) {
        if (c == '"') {
            escaped += "\"\"";
        } else {
            escaped.push_back(c);
        }
    }
    escaped.push_back('"');
    return escaped;
}

}  // namespace

drogon::Task<void> HealthController::addRecord(
    drogon::HttpRequestPtr req,
    std::function<void(const drogon::HttpResponsePtr&)> callback) {
    try {
        const auto userId = req->getAttributes()->get<int64_t>("userId");
        const auto body = req->getJsonObject();
        if (!body) {
            callback(badRequest("El cuerpo debe ser JSON."));
            co_return;
        }

        const double weightKg = (*body).get("weightKg", 0.0).asDouble();
        const double heightCm = (*body).get("heightCm", 0.0).asDouble();
        const int activity = (*body).get("activityLevel", 3).asInt();
        const auto note = (*body).get("note", "").asString();

        if (weightKg <= 0 || weightKg > 600 || heightCm <= 0 || heightCm > 300) {
            callback(badRequest("weightKg y heightCm deben estar en rangos validos."));
            co_return;
        }
        if (activity < 1 || activity > 5) {
            callback(badRequest("activityLevel debe estar entre 1 y 5."));
            co_return;
        }

        const auto bmi = core::BmiCalculator::calculate(weightKg, heightCm);

        core::HealthRecord record;
        record.userId = userId;
        record.weightKg = weightKg;
        record.heightCm = heightCm;
        record.bmi = bmi.bmi;
        record.activityLevel = activity;
        record.note = note;

        const auto repo = AppServices::instance().records();
        const auto saved = co_await repo.insert(record);

        ai::AiEngine engine(AppServices::instance().models(),
                            AppServices::instance().records());
        const auto model = co_await engine.learn(userId, weightKg, heightCm);
        const auto analysis = co_await engine.analyze(userId);

        Json::Value out(Json::objectValue);
        out["record"]["id"] = saved.id;
        out["record"]["weightKg"] = saved.weightKg;
        out["record"]["heightCm"] = saved.heightCm;
        out["record"]["bmi"] = saved.bmi;
        out["record"]["activityLevel"] = saved.activityLevel;
        out["record"]["note"] = saved.note;
        out["record"]["createdAt"] = saved.createdAt;
        out["record"]["procedure"]["heightM"] = bmi.heightMeters;
        out["record"]["procedure"]["heightSquared"] = bmi.heightSquared;
        out["record"]["procedure"]["formula"] =
            "IMC = peso / altura^2 = " + std::to_string(weightKg) + " / (" +
            std::to_string(bmi.heightMeters) + ")^2 = " + std::to_string(bmi.bmi);
        out["record"]["category"] = core::categoryToString(bmi.category);
        out["analysis"] = analysis;

        // Push en tiempo real hacia el dashboard del usuario.
        Json::Value push(Json::objectValue);
        push["event"] = "new_record";
        push["record"] = out["record"];
        push["analysis"] = analysis;
        DashboardWs::broadcastToUser(userId, push);

        callback(created(out));
    } catch (const std::exception& e) {
        LOG_ERROR << "addRecord: " << e.what();
        callback(serverError());
    }
}

drogon::Task<void> HealthController::listRecords(
    drogon::HttpRequestPtr req,
    std::function<void(const drogon::HttpResponsePtr&)> callback) {
    try {
        const auto userId = req->getAttributes()->get<int64_t>("userId");
        size_t limit = 200;
        const auto& limitStr = req->getParameter("limit");
        if (!limitStr.empty()) {
            limit = static_cast<size_t>(std::stoi(limitStr));
        }
        const auto repo = AppServices::instance().records();
        const auto records = co_await repo.listByUser(userId, limit);

        Json::Value out(Json::objectValue);
        Json::Value arr(Json::arrayValue);
        for (const auto& r : records) {
            arr.append(recordToJson(r));
        }
        out["records"] = arr;
        out["count"] = static_cast<int>(arr.size());
        callback(ok(out));
    } catch (const std::exception& e) {
        LOG_ERROR << "listRecords: " << e.what();
        callback(serverError());
    }
}

drogon::Task<void> HealthController::latestRecord(
    drogon::HttpRequestPtr req,
    std::function<void(const drogon::HttpResponsePtr&)> callback) {
    try {
        const auto userId = req->getAttributes()->get<int64_t>("userId");
        const auto repo = AppServices::instance().records();
        const auto latest = co_await repo.latest(userId);
        if (!latest) {
            callback(ok(Json::Value(Json::objectValue)));
            co_return;
        }
        Json::Value item(Json::objectValue);
        item["id"] = latest->id;
        item["weightKg"] = latest->weightKg;
        item["heightCm"] = latest->heightCm;
        item["bmi"] = latest->bmi;
        item["activityLevel"] = latest->activityLevel;
        item["note"] = latest->note;
        item["createdAt"] = latest->createdAt;
        callback(ok(item));
    } catch (const std::exception& e) {
        LOG_ERROR << "latestRecord: " << e.what();
        callback(serverError());
    }
}

drogon::Task<void> HealthController::updateRecord(
    drogon::HttpRequestPtr req,
    std::function<void(const drogon::HttpResponsePtr&)> callback,
    const int64_t& recordId) {
    try {
        const auto userId = req->getAttributes()->get<int64_t>("userId");
        const auto body = req->getJsonObject();
        if (!body) {
            callback(badRequest("El cuerpo debe ser JSON."));
            co_return;
        }

        const double weightKg = (*body).get("weightKg", 0.0).asDouble();
        const double heightCm = (*body).get("heightCm", 0.0).asDouble();
        const int activity = (*body).get("activityLevel", 3).asInt();
        const auto note = (*body).get("note", "").asString();

        if (weightKg <= 0 || weightKg > 600 || heightCm <= 0 || heightCm > 300) {
            callback(badRequest("weightKg y heightCm deben estar en rangos validos."));
            co_return;
        }
        if (activity < 1 || activity > 5) {
            callback(badRequest("activityLevel debe estar entre 1 y 5."));
            co_return;
        }

        const auto repo = AppServices::instance().records();
        const auto existing = co_await repo.findById(userId, recordId);
        if (!existing) {
            callback(notFound("Registro no encontrado."));
            co_return;
        }

        core::HealthRecord record = *existing;
        record.weightKg = weightKg;
        record.heightCm = heightCm;
        record.bmi = core::BmiCalculator::calculate(weightKg, heightCm).bmi;
        record.activityLevel = activity;
        record.note = note;

        const auto saved = co_await repo.update(record);

        ai::AiEngine engine(AppServices::instance().models(),
                            AppServices::instance().records());
        co_await engine.rebuild(userId);
        const auto analysis = co_await engine.analyze(userId);

        Json::Value out(Json::objectValue);
        out["record"] = recordToJson(saved);
        out["analysis"] = analysis;

        Json::Value push(Json::objectValue);
        push["event"] = "update_record";
        push["record"] = out["record"];
        push["analysis"] = analysis;
        DashboardWs::broadcastToUser(userId, push);

        callback(ok(out));
    } catch (const std::exception& e) {
        LOG_ERROR << "updateRecord: " << e.what();
        callback(serverError());
    }
}

drogon::Task<void> HealthController::deleteRecord(
    drogon::HttpRequestPtr req,
    std::function<void(const drogon::HttpResponsePtr&)> callback,
    const int64_t& recordId) {
    try {
        const auto userId = req->getAttributes()->get<int64_t>("userId");
        const auto repo = AppServices::instance().records();
        const bool removed = co_await repo.remove(userId, recordId);
        if (!removed) {
            callback(notFound("Registro no encontrado."));
            co_return;
        }

        ai::AiEngine engine(AppServices::instance().models(),
                            AppServices::instance().records());
        co_await engine.rebuild(userId);
        const auto analysis = co_await engine.analyze(userId);

        Json::Value out(Json::objectValue);
        out["ok"] = true;
        out["analysis"] = analysis;

        Json::Value push(Json::objectValue);
        push["event"] = "delete_record";
        push["recordId"] = recordId;
        push["analysis"] = analysis;
        DashboardWs::broadcastToUser(userId, push);

        callback(ok(out));
    } catch (const std::exception& e) {
        LOG_ERROR << "deleteRecord: " << e.what();
        callback(serverError());
    }
}

drogon::Task<void> HealthController::exportRecords(
    drogon::HttpRequestPtr req,
    std::function<void(const drogon::HttpResponsePtr&)> callback) {
    try {
        const auto userId = req->getAttributes()->get<int64_t>("userId");
        const auto repo = AppServices::instance().records();
        const auto records = co_await repo.listByUser(userId, 100000);

        std::ostringstream csv;
        csv << "fecha,peso_kg,altura_cm,imc,actividad,nota\n";
        for (const auto& r : records) {
            csv << r.createdAt << ','
                << r.weightKg << ','
                << r.heightCm << ','
                << r.bmi << ','
                << r.activityLevel << ','
                << csvEscape(r.note) << '\n';
        }

        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setBody(csv.str());
        resp->setContentTypeString("text/csv; charset=utf-8");
        resp->addHeader("Content-Disposition",
                        "attachment; filename=\"healthiq_records.csv\"");
        callback(resp);
    } catch (const std::exception& e) {
        LOG_ERROR << "exportRecords: " << e.what();
        callback(serverError());
    }
}

}  // namespace healthiq::api
