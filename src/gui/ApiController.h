/**
 * @file ApiController.h
 * @brief JSON API behind the browser board. Translates HTTP-style requests into
 *        calls on the WorkflowEngineFacade.
 *
 * It knows nothing about sockets. The web server (app/gui_main.cpp) only forwards
 * method, path and body here. It never sees a Task* or Project*: everything crossing
 * the Facade is ids and JSON.
 *
 * Routes (all bodies are JSON):
 *   GET  /api/definitions                          stored workflow definitions
 *   GET  /api/actions                              registered action names
 *   GET  /api/projects                             running projects with progress
 *   POST /api/projects                             {definitionId?, name, description?}
 *   POST /api/projects/import                      an exported project (wrapped or bare); progress resets to Pending
 *   GET  /api/projects/:pid                        snapshot: project tree, work, history, undo/redo
 *   GET  /api/projects/:pid/export                 {format, version, project}
 *   POST /api/projects/:pid/clone                  {name}
 *   POST /api/projects/:pid/undo | /redo
 *   POST /api/projects/:pid/tasks                  {name, description?}
 *   POST /api/projects/:pid/dependencies           {dependent, prerequisite}
 *   POST /api/projects/:pid/tasks/:tid/actions     {action, args?}
 *   POST /api/projects/:pid/tasks/:tid/edit        edit payload (see EditTaskInfoCommand)
 *   POST /api/projects/:pid/tasks/:tid/assign      {id, name, role}
 *   POST /api/projects/:pid/tasks/:tid/decide      {context}
 *   POST /api/projects/:pid/tasks/:tid/duplicate   copy as a new Pending task (no dependencies); undoable
 *   POST /api/projects/:pid/tasks/:tid/delete      remove the task; undoable
 * Every mutating project route answers with a fresh snapshot. In snapshots every task also carries
 * "ready" (from the engine's own readiness), "executor": null when unassigned, "deadline": "" when unset and
 * "failedBecause": ids of failed prerequisites for a task that failed upstream.
 * Edit deadlines are checked against the calendar; "deadline": null clears one.
 */
#ifndef TIVIDY_APICONTROLLER_H
#define TIVIDY_APICONTROLLER_H

#include <string>
#include <vector>

#include "nlohmann/json.hpp"

class WorkflowEngineFacade;

/** @brief HTTP status code plus JSON body. */
struct ApiResponse {
    int status;
    nlohmann::json body;
    ApiResponse(int status, const nlohmann::json& body) : status(status), body(body) {}
};

class ApiController {
public:
    explicit ApiController(WorkflowEngineFacade& engine);

    /**
     * @brief Dispatches one request. Never throws for domain errors:
     * NotFoundError -> 404, other WorkflowError -> 422, malformed JSON -> 400,
     * unknown route -> 404, anything else unexpected -> 500.
     * Error bodies look like {"error": "..."}.
     */
    ApiResponse handle(const std::string& method, const std::string& path, const std::string& body);

private:
    typedef std::vector<std::string> Params;
    typedef nlohmann::json (ApiController::*Handler)(const Params& params, const nlohmann::json& body);
    struct Route {
        std::string method;
        std::vector<std::string> pattern;  // literal segments, or ":name" placeholders
        Handler handler;
        int successStatus;
    };

    void route(const std::string& method, const std::string& pattern, Handler handler, int successStatus = 200);
    static bool match(const Route& r, const std::string& method, const std::vector<std::string>& segments,
                      Params& params);

    nlohmann::json snapshot(const std::string& projectId) const;

    nlohmann::json getDefinitions(const Params&, const nlohmann::json&);
    nlohmann::json getActions(const Params&, const nlohmann::json&);
    nlohmann::json getProjects(const Params&, const nlohmann::json&);
    nlohmann::json createProject(const Params&, const nlohmann::json& body);
    nlohmann::json getProject(const Params& p, const nlohmann::json&);
    nlohmann::json cloneProject(const Params& p, const nlohmann::json& body);
    nlohmann::json undo(const Params& p, const nlohmann::json&);
    nlohmann::json redo(const Params& p, const nlohmann::json&);
    nlohmann::json addTask(const Params& p, const nlohmann::json& body);
    nlohmann::json addDependency(const Params& p, const nlohmann::json& body);
    nlohmann::json performAction(const Params& p, const nlohmann::json& body);
    nlohmann::json editTask(const Params& p, const nlohmann::json& body);
    nlohmann::json assign(const Params& p, const nlohmann::json& body);
    nlohmann::json decide(const Params& p, const nlohmann::json& body);
    nlohmann::json duplicateTask(const Params& p, const nlohmann::json&);
    nlohmann::json deleteTask(const Params& p, const nlohmann::json&);
    nlohmann::json exportProject(const Params& p, const nlohmann::json&);
    nlohmann::json importProject(const Params&, const nlohmann::json& body);

    WorkflowEngineFacade& engine;
    std::vector<Route> routes;
};

#endif
