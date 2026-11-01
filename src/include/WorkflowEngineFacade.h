#ifndef TIVIDY_WORKFLOW_ENGINE_FACADE_H
#define TIVIDY_WORKFLOW_ENGINE_FACADE_H

#include "ProjectManagement.h"
#include "Execution.h"
#include "Reporting.h"
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

class ProjectRegistry;
class WorkflowFactory;
class ReportService;
class DefinitionRepository;
class ActionRegistry;
class RouterRegistry;
class Participant;
struct WorkItemView;

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

    /**
     * @brief Creates and registers an empty project.
     * @param name Human-readable project name.
     * @param desc Optional project description.
     * @return The unique identifier of the new project.
     * @throws WorkflowError If the project cannot be created or registered.
     */
    std::string createProject(std::string name, std::string desc = "") override;

    /**
     * @brief Creates and registers a project from a JSON workflow template.
     * @param templateJson JSON object describing the project and its work items.
     * @return The unique identifier of the new project.
     * @throws WorkflowError If the template is invalid or project registration fails.
     */
    std::string createFromTemplate(const nlohmann::json& templateJson) override;

    /**
     * @brief Creates an instance from a registered workflow definition.
     * @param definitionId Identifier of the workflow definition.
     * @param instanceName Name for the new project instance.
     * @return The unique identifier of the new project.
     * @throws NotFoundError If no definition has the given identifier.
     * @throws WorkflowError If the definition cannot be instantiated or registered.
     */
    std::string startInstance(std::string definitionId, std::string instanceName) override;

    /**
     * @brief Adds a task to an existing project.
     * @param projectId Identifier of the project to update.
     * @param name Human-readable task name.
     * @param desc Optional task description.
     * @return The unique identifier of the new task.
     * @throws NotFoundError If the project does not exist.
     */
    std::string addTaskToProject(std::string projectId, std::string name, std::string desc = "") override;

    /**
     * @brief Adds a prerequisite relationship between two project components.
     * @param projectId Identifier of the project containing the components.
     * @param dependentId Identifier of the component that depends on the prerequisite.
     * @param prerequisiteId Identifier of the component that must be satisfied first.
     * @throws NotFoundError If the project or either component does not exist.
     * @throws CyclicDependencyError If the relationship would create a dependency cycle.
     */
    void addDependency(std::string projectId, std::string dependentId, std::string prerequisiteId) override;

    /**
     * @brief Assigns a participant as the executor of a task.
     * @param projectId Identifier of the project containing the task.
     * @param taskId Identifier of the task to update.
     * @param p Participant to assign.
     * @throws NotFoundError If the project or task does not exist.
     */
    void assignExecutor(std::string projectId, std::string taskId, const Participant& p) override;

    /**
     * @brief Lists the identifiers of all registered projects.
     * @return Project identifiers.
     */
    std::vector<std::string> listProjects() const override;

    /**
     * @brief Clones a project and registers the clone under a new name.
     * @param projectId Identifier of the project to clone.
     * @param newName Name for the cloned project.
     * @return The unique identifier of the clone.
     * @throws NotFoundError If the source project does not exist.
     * @throws WorkflowError If the clone cannot be registered.
     */
    std::string cloneProject(std::string projectId, std::string newName);

    /**
     * @brief Lists the registered workflow definitions.
     * @return A JSON array of definition objects containing their IDs, names, industries, and descriptions.
     */
    nlohmann::json listDefinitions() const;

    /**
     * @brief Creates a project instance from a registered definition.
     * @param definitionId Identifier of the definition to instantiate.
     * @param instanceName Optional name for the new project; an empty name keeps the template name.
     * @return The unique identifier of the new project.
     * @throws NotFoundError If no definition has the given identifier.
     * @throws WorkflowError If the project cannot be imported or registered.
     */
    std::string instantiateDefinition(std::string definitionId, std::string instanceName);

    /**
     * @brief Imports a project from its exported JSON representation.
     * @details Rebuilds the work items, dependencies, and decision branches, and assigns
     *          fresh identifiers to items whose IDs were generated by this engine.
     * @param projectJson Exported project JSON, optionally wrapped in a \c project object.
     * @return The unique identifier of the imported project.
     * @throws WorkflowError If the JSON is malformed, references unknown items or routers,
     *         contains an invalid deadline, or cannot be registered.
     */
    std::string importProject(const nlohmann::json& projectJson);

    /**
     * @brief Adds a copy of a task to the project.
     * @details The copy receives a new identifier and is added as a project work item.
     *          Decision tasks are not supported because their branch mappings are not copied.
     * @param projectId Identifier of the project containing the source task.
     * @param taskId Identifier of the task to duplicate.
     * @return The unique identifier of the new task.
     * @throws NotFoundError If the project or task does not exist.
     * @throws WorkflowError If the task is a decision task or the copy cannot be added.
     */
    std::string duplicateTask(std::string projectId, std::string taskId);

    /**
     * @brief Removes a work item from a project.
     * @param projectId Identifier of the project to update.
     * @param taskId Identifier of the work item to remove.
     * @throws NotFoundError If the project or work item does not exist.
     * @throws WorkflowError If the project itself is requested for removal or removal fails.
     */
    void removeTask(std::string projectId, std::string taskId);

    /**
     * @brief Checks whether date and time fields form a valid calendar date and time.
     * @param year Calendar year, from 1900 through 9999.
     * @param month Month of the year, from 1 through 12.
     * @param day Day of the month, accounting for month lengths and leap years.
     * @param hour Hour of the day, from 0 through 23.
     * @param minute Minute of the hour, from 0 through 59.
     * @return \c true if all fields represent a valid date and time; otherwise \c false.
     */
    static bool isValidDateTime(int year, int month, int day, int hour, int minute);

    /**
     * @brief Performs a registered action on a task.
     * @param projectId Identifier of the project containing the task.
     * @param taskId Identifier of the target task.
     * @param action Name of the registered action to perform.
     * @param args Optional JSON arguments passed to the action.
     * @throws NotFoundError If the project, task, or action does not exist.
     * @throws InvalidTransitionError If the action is invalid for the task's current state.
     */
    void performAction(std::string projectId, std::string taskId, std::string action, const nlohmann::json& args = {}) override;

    /**
     * @brief Applies a JSON patch to a task's editable information.
     * @param projectId Identifier of the project containing the task.
     * @param taskId Identifier of the task to edit.
     * @param payload JSON object containing fields to update.
     * @throws NotFoundError If the project or task does not exist.
     * @throws WorkflowError If the payload is invalid or contains an unsupported field.
     */
    void editTask(std::string projectId, std::string taskId, const nlohmann::json& payload) override;

    /**
     * @brief Evaluates a decision task and applies the selected branch.
     * @param projectId Identifier of the project containing the decision.
     * @param taskId Identifier of the decision task.
     * @param context JSON context consumed by the decision's routing strategy.
     * @return The label of the selected branch.
     * @throws NotFoundError If the project, task, router, or branch does not exist.
     * @throws WorkflowError If the target task is not a decision task.
     * @throws InvalidTransitionError If the decision has already been made.
     */
    std::string decide(std::string projectId, std::string taskId, const nlohmann::json& context) override;

    /**
     * @brief Undoes the most recent command for a project.
     * @param projectId Identifier of the project.
     * @throws NotFoundError If the project does not exist.
     * @throws WorkflowError If there is no command to undo.
     */
    void undo(std::string projectId) override;

    /**
     * @brief Redoes the most recently undone command for a project.
     * @param projectId Identifier of the project.
     * @throws NotFoundError If the project does not exist.
     * @throws WorkflowError If there is no command to redo.
     */
    void redo(std::string projectId) override;

    /**
     * @brief Checks whether a project has a command available to undo.
     * @param projectId Identifier of the project.
     * @return \c true if an undo operation is available; otherwise \c false.
     * @throws NotFoundError If the project does not exist.
     */
    bool canUndo(std::string projectId) const override;

    /**
     * @brief Checks whether a project has a command available to redo.
     * @param projectId Identifier of the project.
     * @return \c true if a redo operation is available; otherwise \c false.
     * @throws NotFoundError If the project does not exist.
     */
    bool canRedo(std::string projectId) const override;

    /**
     * @brief Lists the action names registered with the action registry.
     * @return Names of the registered actions.
     */
    std::vector<std::string> listActions() const;

    /**
     * @brief Generates a human-readable summary of a project's status.
     * @param projectId Identifier of the project.
     * @return Formatted status and completion summary.
     * @throws NotFoundError If the project does not exist.
     */
    std::string viewProjectStatus(std::string projectId) const override;

    /**
     * @brief Exports a project's structure and work item data as JSON.
     * @param projectId Identifier of the project.
     * @return JSON representation of the project.
     * @throws NotFoundError If the project does not exist.
     */
    nlohmann::json exportProjectToJson(std::string projectId) const override;

    /**
     * @brief Lists work items currently available to start or continue.
     * @param projectId Identifier of the project.
     * @return Work item views for available work.
     * @throws NotFoundError If the project does not exist.
     */
    std::vector<WorkItemView> listAvailableWork(std::string projectId) const override;

    /**
     * @brief Retrieves the audit history for a project.
     * @param projectId Identifier of the project.
     * @return Formatted audit entries in chronological order.
     * @throws NotFoundError If the project does not exist.
     */
    std::vector<std::string> getHistory(std::string projectId) const override;
};

#endif 