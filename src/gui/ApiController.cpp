#include "gui/ApiController.h"

#include <cstdio>
#include <map>
#include <set>
#include <sstream>

#include "Support.h"
#include "WorkflowEngineFacade.h"
#include "WorkflowError.h"

namespace {

std::vector<std::string> splitPath(const std::string& path) {
    std::vector<std::string> out;
    std::string clean = path.substr(0, path.find('?'));
    std::istringstream in(clean);
    std::string part;
    while (std::getline(in, part, '/')) {
        if (!part.empty()) out.push_back(part);
    }
    return out;
}

std::string requireString(const nlohmann::json& body, const std::string& key) {
    if (!body.is_object() || !body.contains(key) || !body[key].is_string() || body[key].get<std::string>().empty()) {
        throw WorkflowError("Request needs a non-empty string '" + key + "'");
    }
    return body[key].get<std::string>();
}

std::string optionalString(const nlohmann::json& body, const std::string& key) {
    if (!body.is_object() || !body.contains(key)) return "";
    if (!body[key].is_string()) throw WorkflowError("'" + key + "' must be a string");
    return body[key].get<std::string>();
}

nlohmann::json optionalObject(const nlohmann::json& body, const std::string& key) {
    if (!body.is_object() || !body.contains(key) || body[key].is_null()) return nlohmann::json::object();
    if (!body[key].is_object()) throw WorkflowError("'" + key + "' must be an object");
    return body[key];
}

void countLeaves(const nlohmann::json& node, int& done, int& total) {
    if (node.contains("workItems")) {
        for (std::size_t i = 0; i < node["workItems"].size(); ++i) countLeaves(node["workItems"][i], done, total);
        return;
    }
    ++total;
    std::string status = node.value("status", "");
    if (status == "Complete" || status == "Skipped") ++done;
}

nlohmann::json errorBody(const std::string& message) {
    nlohmann::json j;
    j["error"] = message;
    return j;
}

nlohmann::json deadlineJson(int year, int month, int day, int hour, int minute) {
    nlohmann::json d;
    d["year"] = year;
    d["month"] = month;
    d["day"] = day;
    d["hour"] = hour;
    d["min"] = minute;
    return d;
}

/**
 * Fills in what the board needs but the engine's task JSON leaves out or encodes awkwardly:
 * "ready" (taken from the engine's own list of available work), "executor": null when nobody is assigned,
 * and "deadline": "" when none is set (the engine writes 0000-00-00 00:00).
 */
void collectStatuses(const nlohmann::json& node, std::map<std::string, std::string>& out) {
    if (node.contains("workItems") && node["workItems"].is_array()) {
        for (std::size_t i = 0; i < node["workItems"].size(); ++i) collectStatuses(node["workItems"][i], out);
    }
    if (node.contains("id") && node.contains("status")) out[node["id"].get<std::string>()] = node["status"].get<std::string>();
}

void decorate(nlohmann::json& node, const std::set<std::string>& available,
              const std::map<std::string, std::string>& statusById) {
    if (node.contains("workItems") && node["workItems"].is_array()) {
        for (std::size_t i = 0; i < node["workItems"].size(); ++i) decorate(node["workItems"][i], available, statusById);
    }
    if (node.value("type", std::string()) == "Project") {
        node["ready"] = true;
        return;
    }
    if (node.contains("executor") && node["executor"].is_object() &&
        node["executor"].value("id", std::string()).empty()) {
        node["executor"] = nullptr;
    }
    if (node.value("deadline", std::string()).compare(0, 4, "0000") == 0) node["deadline"] = "";

    std::string status = node.value("status", std::string());
    bool finished = status == "Complete" || status == "Skipped" || status == "Cancelled";
    node["ready"] = finished || available.count(node.value("id", std::string())) > 0;

    // A task that failed because a prerequisite failed: say which ones.
    nlohmann::json failedBecause = nlohmann::json::array();
    if (status == "Failed" && node.contains("dependsOn") && node["dependsOn"].is_array()) {
        for (std::size_t i = 0; i < node["dependsOn"].size(); ++i) {
            std::string pre = node["dependsOn"][i].get<std::string>();
            std::map<std::string, std::string>::const_iterator found = statusById.find(pre);
            if (found != statusById.end() && found->second == "Failed") failedBecause.push_back(pre);
        }
    }
    node["failedBecause"] = failedBecause;
}

/**
 * Checks an edit payload's deadline against the calendar before the engine sees it, and turns the
 * accepted forms (object, "YYYY-MM-DD[ HH:MM]" text, or null/"" to clear) into the object the engine expects.
 */
nlohmann::json checkedEdit(const nlohmann::json& body) {
    nlohmann::json payload = body;
    if (!payload.is_object() || !payload.contains("deadline")) return payload;

    const nlohmann::json& d = payload["deadline"];
    if (d.is_null() || (d.is_string() && d.get<std::string>().empty())) {
        payload["deadline"] = deadlineJson(0, 0, 0, 0, 0);  // the engine's "no deadline"
        return payload;
    }

    int y = 0, mo = 0, day = 0, h = 0, mi = 0;
    if (d.is_object()) {
        y = d.at("year").get<int>();
        mo = d.at("month").get<int>();
        day = d.at("day").get<int>();
        h = d.value("hour", 0);
        mi = d.value("min", 0);
    } else if (d.is_string()) {
        int got = std::sscanf(d.get<std::string>().c_str(), "%d-%d-%d %d:%d", &y, &mo, &day, &h, &mi);
        if (got != 3 && got != 5) throw WorkflowError("Invalid deadline: use YYYY-MM-DD or YYYY-MM-DD HH:MM");
    } else {
        throw WorkflowError("'deadline' must be an object, a date string, or null");
    }

    if (!WorkflowEngineFacade::isValidDateTime(y, mo, day, h, mi)) {
        char text[64];
        std::snprintf(text, sizeof text, "%04d-%02d-%02d %02d:%02d", y, mo, day, h, mi);
        throw WorkflowError(std::string("Invalid deadline ") + text + ": that date or time does not exist");
    }
    payload["deadline"] = deadlineJson(y, mo, day, h, mi);
    return payload;
}

}  // namespace

ApiController::ApiController(WorkflowEngineFacade& engine) : engine(engine) {
    route("GET", "/api/definitions", &ApiController::getDefinitions);
    route("GET", "/api/actions", &ApiController::getActions);
    route("GET", "/api/projects", &ApiController::getProjects);
    route("POST", "/api/projects", &ApiController::createProject, 201);
    route("POST", "/api/projects/import", &ApiController::importProject, 201);
    route("GET", "/api/projects/:pid", &ApiController::getProject);
    route("GET", "/api/projects/:pid/export", &ApiController::exportProject);
    route("POST", "/api/projects/:pid/clone", &ApiController::cloneProject, 201);
    route("POST", "/api/projects/:pid/undo", &ApiController::undo);
    route("POST", "/api/projects/:pid/redo", &ApiController::redo);
    route("POST", "/api/projects/:pid/tasks", &ApiController::addTask, 201);
    route("POST", "/api/projects/:pid/dependencies", &ApiController::addDependency);
    route("POST", "/api/projects/:pid/tasks/:tid/actions", &ApiController::performAction);
    route("POST", "/api/projects/:pid/tasks/:tid/edit", &ApiController::editTask);
    route("POST", "/api/projects/:pid/tasks/:tid/assign", &ApiController::assign);
    route("POST", "/api/projects/:pid/tasks/:tid/decide", &ApiController::decide);
    route("POST", "/api/projects/:pid/tasks/:tid/duplicate", &ApiController::duplicateTask, 201);
    route("POST", "/api/projects/:pid/tasks/:tid/delete", &ApiController::deleteTask);
}

void ApiController::route(const std::string& method, const std::string& pattern, Handler handler,
                          int successStatus) {
    Route r;
    r.method = method;
    r.pattern = splitPath(pattern);
    r.handler = handler;
    r.successStatus = successStatus;
    routes.push_back(r);
}

bool ApiController::match(const Route& r, const std::string& method, const std::vector<std::string>& segments,
                          Params& params) {
    if (r.method != method || r.pattern.size() != segments.size()) return false;
    Params found;
    for (std::size_t i = 0; i < segments.size(); ++i) {
        if (r.pattern[i][0] == ':') {
            found.push_back(segments[i]);
        } else if (r.pattern[i] != segments[i]) {
            return false;
        }
    }
    params = found;
    return true;
}

ApiResponse ApiController::handle(const std::string& method, const std::string& path, const std::string& body) {
    std::vector<std::string> segments = splitPath(path);
    for (std::size_t i = 0; i < routes.size(); ++i) {
        Params params;
        if (!match(routes[i], method, segments, params)) continue;
        try {
            nlohmann::json parsed = body.empty() ? nlohmann::json::object() : nlohmann::json::parse(body);
            return ApiResponse(routes[i].successStatus, (this->*(routes[i].handler))(params, parsed));
        } catch (const NotFoundError& e) {
            return ApiResponse(404, errorBody(e.what()));
        } catch (const WorkflowError& e) {
            return ApiResponse(422, errorBody(e.what()));
        } catch (const nlohmann::json::exception& e) {
            return ApiResponse(400, errorBody(std::string("Invalid JSON: ") + e.what()));
        } catch (const std::exception& e) {
            return ApiResponse(500, errorBody(std::string("Unexpected error: ") + e.what()));
        }
    }
    return ApiResponse(404, errorBody("No route for " + method + " " + path));
}

nlohmann::json ApiController::snapshot(const std::string& projectId) const {
    nlohmann::json j;
    std::vector<WorkItemView> work = engine.listAvailableWork(projectId);
    std::set<std::string> available;
    for (std::size_t i = 0; i < work.size(); ++i) available.insert(work[i].id);
    nlohmann::json project = engine.exportProjectToJson(projectId);
    std::map<std::string, std::string> statusById;
    collectStatuses(project, statusById);
    decorate(project, available, statusById);
    j["project"] = project;
    j["work"] = nlohmann::json::array();
    for (std::size_t i = 0; i < work.size(); ++i) {
        nlohmann::json w;
        w["id"] = work[i].id;
        w["name"] = work[i].name;
        w["status"] = work[i].status;
        w["executor"] = work[i].executor;
        w["ready"] = work[i].ready;
        j["work"].push_back(w);
    }
    j["history"] = engine.getHistory(projectId);
    j["canUndo"] = engine.canUndo(projectId);
    j["canRedo"] = engine.canRedo(projectId);
    return j;
}

nlohmann::json ApiController::getDefinitions(const Params&, const nlohmann::json&) {
    return engine.listDefinitions();
}

nlohmann::json ApiController::getActions(const Params&, const nlohmann::json&) {
    return engine.listActions();
}

nlohmann::json ApiController::getProjects(const Params&, const nlohmann::json&) {
    nlohmann::json out = nlohmann::json::array();
    std::vector<std::string> ids = engine.listProjects();
    for (std::size_t i = 0; i < ids.size(); ++i) {
        nlohmann::json p = engine.exportProjectToJson(ids[i]);
        int done = 0;
        int total = 0;
        countLeaves(p, done, total);
        nlohmann::json row;
        row["id"] = ids[i];
        row["name"] = p["name"];
        row["completed"] = p["completed"];
        row["industry"] = p["customFields"].value("industry", "");
        row["done"] = done;
        row["total"] = total;
        out.push_back(row);
    }
    return out;
}

nlohmann::json ApiController::createProject(const Params&, const nlohmann::json& body) {
    std::string definitionId = optionalString(body, "definitionId");
    std::string id = definitionId.empty()
                         ? engine.createProject(requireString(body, "name"), optionalString(body, "description"))
                         : engine.instantiateDefinition(definitionId, optionalString(body, "name"));
    return snapshot(id);
}

nlohmann::json ApiController::getProject(const Params& p, const nlohmann::json&) { return snapshot(p[0]); }

nlohmann::json ApiController::cloneProject(const Params& p, const nlohmann::json& body) {
    return snapshot(engine.cloneProject(p[0], optionalString(body, "name")));
}

nlohmann::json ApiController::undo(const Params& p, const nlohmann::json&) {
    engine.undo(p[0]);
    return snapshot(p[0]);
}

nlohmann::json ApiController::redo(const Params& p, const nlohmann::json&) {
    engine.redo(p[0]);
    return snapshot(p[0]);
}

nlohmann::json ApiController::addTask(const Params& p, const nlohmann::json& body) {
    engine.addTaskToProject(p[0], requireString(body, "name"), optionalString(body, "description"));
    return snapshot(p[0]);
}

nlohmann::json ApiController::addDependency(const Params& p, const nlohmann::json& body) {
    engine.addDependency(p[0], requireString(body, "dependent"), requireString(body, "prerequisite"));
    return snapshot(p[0]);
}

nlohmann::json ApiController::performAction(const Params& p, const nlohmann::json& body) {
    engine.performAction(p[0], p[1], requireString(body, "action"), optionalObject(body, "args"));
    return snapshot(p[0]);
}

nlohmann::json ApiController::editTask(const Params& p, const nlohmann::json& body) {
    engine.editTask(p[0], p[1], checkedEdit(body));
    return snapshot(p[0]);
}

nlohmann::json ApiController::assign(const Params& p, const nlohmann::json& body) {
    engine.assignExecutor(p[0], p[1],
                            Participant(requireString(body, "id"), requireString(body, "name"),
                                        optionalString(body, "role")));
    return snapshot(p[0]);
}

nlohmann::json ApiController::decide(const Params& p, const nlohmann::json& body) {
    std::string branch = engine.decide(p[0], p[1], optionalObject(body, "context"));
    nlohmann::json j = snapshot(p[0]);
    j["branch"] = branch;
    return j;
}

nlohmann::json ApiController::duplicateTask(const Params& p, const nlohmann::json&) {
    engine.duplicateTask(p[0], p[1]);
    return snapshot(p[0]);
}

nlohmann::json ApiController::deleteTask(const Params& p, const nlohmann::json&) {
    engine.removeTask(p[0], p[1]);
    return snapshot(p[0]);
}

nlohmann::json ApiController::exportProject(const Params& p, const nlohmann::json&) {
    nlohmann::json j;
    j["format"] = "tividy-project";
    j["version"] = 1;
    j["project"] = engine.exportProjectToJson(p[0]);
    return j;
}

nlohmann::json ApiController::importProject(const Params&, const nlohmann::json& body) {
    return snapshot(engine.importProject(body));
}
