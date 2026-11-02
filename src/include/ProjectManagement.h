#ifndef TIVIDY_PROJECT_MANAGEMENT_H
#define TIVIDY_PROJECT_MANAGEMENT_H

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

// Forward declarations
class Participant;

/**
 * @brief Interface for managing project creation, structural definitions, and component wiring.
 * 
 * This interface exposes high-level administrative actions for initialising projects,
 * adding tasks, establishing dependencies, and assigning resources.
 */
class ProjectManagement {
public:
    /**
     * @brief Virtual destructor for the interface.
     */
    virtual ~ProjectManagement() = default;

    /**
     * @brief Creates a brand new, empty project container.
     * 
     * @details Allocates a new project instance with a generated unique identifier 
     *          and registers it into the active project registry session store.
     * 
     * @param name The human-readable name of the project.
     * @param desc An optional detailed description of the project scope.
     * @return `std::string` The unique identifier assigned to the newly created project.
     * @throws `WorkflowError` If project initialisation or registry insertion fails.
     */
    virtual std::string createProject(std::string name, std::string desc = "") = 0;

    /**
     * @brief Creates a project instance dynamically from a JSON template.
     * 
     * @details Parses the provided template structure using the workflow factory and 
     *          director, instantiating a fully wired component graph with fresh IDs.
     * 
     * @param templateJson A JSON object containing the structural template definition.
     * @return `std::string` The unique identifier of the newly instantiated project.
     * @throws `WorkflowError` If the JSON template is malformed or missing required fields.
     */
    virtual std::string createFromTemplate(const nlohmann::json& templateJson) = 0;

    /**
     * @brief Starts a new project instance from a registered workflow definition.
     * 
     * @details Looks up a pre-configured definition by its identifier in the definition 
     *          repository and instantiates an independent running project session.
     * 
     * @param definitionId The unique identifier of the registered workflow definition.
     * @param instanceName The name to assign to this specific running instance.
     * @return `std::string` The unique identifier of the newly spawned project session.
     * @throws `NotFoundError` If the `definitionId` does not exist in the repository.
     */
    virtual std::string startInstance(std::string definitionId, std::string instanceName) = 0;

    /**
     * @brief Appends a new task to an existing project.
     * 
     * @details Locates the target project via its identifier and creates a new child task 
     *          wrapped inside an `AddWorkItemCommand` for history tracking.
     * 
     * @param projectId The unique identifier of the parent project.
     * @param name The name of the new task.
     * @param desc An optional description for the task.
     * @return `std::string` The unique identifier of the newly created task.
     * @throws `NotFoundError` If the project ID cannot be located.
     */
    virtual std::string addTaskToProject(std::string projectId, std::string name, std::string desc = "") = 0;

    /**
     * @brief Establishes a prerequisite dependency between two components.
     * 
     * @details Links a prerequisite work item to a dependent work item. Validates that 
     *          the operation does not introduce cyclic dependencies.
     * 
     * @param projectId The unique identifier of the project containing the components.
     * @param dependentId The identifier of the component that requires the prerequisite.
     * @param prerequisiteId The identifier of the component that must finish first.
     * @throws `NotFoundError` If the project or component IDs are invalid.
     * @throws `CyclicDependencyError` If adding this dependency creates a circular graph.
     */
    virtual void addDependency(std::string projectId, std::string dependentId, std::string prerequisiteId) = 0;

    /**
     * @brief Assigns a designated participant executor to a specific task.
     * 
     * @details Updates the ownership/execution target for a task within the project graph.
     * 
     * @param projectId The unique identifier of the project.
     * @param taskId The unique identifier of the target task.
     * @param p The participant object representing the executor.
     * @throws `NotFoundError` If the project or task cannot be found.
     */
    virtual void assignExecutor(std::string projectId, std::string taskId, const Participant& p) = 0;

    /**
     * @brief Retrieves a list of all active project identifiers.
     * 
     * @return `std::vector<std::string>` A collection of unique project IDs currently managed.
     */
    virtual std::vector<std::string> listProjects() const = 0;
};

#endif // TIVIDY_PROJECT_MANAGEMENT_H