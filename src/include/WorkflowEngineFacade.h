#ifndef TIVIDY_WORKFLOW_ENGINE_FACADE_H
#define TIVIDY_WORKFLOW_ENGINE_FACADE_H

#include "ProjectManagement.h"
#include "Execution.h"
#include "Reporting.h"
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

// Forward declarations of core subsystem dependencies
class ProjectRegistry;
class WorkflowFactory;
class ReportService;
class DefinitionRepository;
class ActionRegistry;
class RouterRegistry;
class Participant;
class WorkItemView;

/**
 * @brief Concrete facade entry point coordinating all workflow engine subsystems.
 * 
 * Implements the `ProjectManagement`, `Execution`, and `Reporting` interfaces. 
 * Acts as a thin delegator connecting incoming client requests 
 * to registries, factories, report generators, and command handlers without maintaining business state.
 */
class WorkflowEngineFacade : public ProjectManagement, public Execution, public Reporting {
private:
    ProjectRegistry& registry;             ///< Reference to the project session registry.
    WorkflowFactory& factory;             ///< Reference to the component creation factory.
    ReportService& reports;               ///< Reference to the reporting generation service.
    DefinitionRepository& definitions;    ///< Reference to the workflow definition repository.
    ActionRegistry& actions;              ///< Reference to the action creator registry.
    RouterRegistry& routers;              ///< Reference to the routing strategy registry.

public:
    /**
     * @brief Constructs the workflow engine facade with required service dependencies.
     * 
     * @param reg Reference to the active project session registry.
     * @param f Reference to the workflow creation factory.
     * @param r Reference to the reporting service.
     * @param d Reference to the definition repository.
     * @param a Reference to the action registry.
     * @param rr Reference to the router registry.
     */
    WorkflowEngineFacade(ProjectRegistry& reg, WorkflowFactory& f, ReportService& r, 
                           DefinitionRepository& d, ActionRegistry& a, RouterRegistry& rr);


    // ProjectManagement Interface Implementation 
    std::string createProject(std::string name, std::string desc = "") override;
    std::string createFromTemplate(const nlohmann::json& templateJson) override;
    std::string startInstance(std::string definitionId, std::string instanceName) override;
    std::string addTaskToProject(std::string projectId, std::string name, std::string desc = "") override;
    void addDependency(std::string projectId, std::string dependentId, std::string prerequisiteId) override;
    void assignExecutor(std::string projectId, std::string taskId, const Participant& p) override;
    std::vector<std::string> listProjects() const override;

    // Execution Interface Implementation 
    void performAction(std::string projectId, std::string taskId, std::string action, const nlohmann::json& args = {}) override;
    void editTask(std::string projectId, std::string taskId, const nlohmann::json& payload) override;
    std::string decide(std::string projectId, std::string taskId, const nlohmann::json& context) override;
    void undo(std::string projectId) override;
    void redo(std::string projectId) override;
    bool canUndo(std::string projectId) const override;
    bool canRedo(std::string projectId) const override;

    //  Reporting Interface Implementation 
    std::string viewProjectStatus(std::string projectId) const override;
    nlohmann::json exportProjectToJson(std::string projectId) const override;
    std::vector<WorkItemView> listAvailableWork(std::string projectId) const override;
    std::vector<std::string> getHistory(std::string projectId) const override;
};

#endif // TIVIDY_WORKFLOW_ENGINE_FACADE_H