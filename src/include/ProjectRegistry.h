#ifndef TIVIDY_PROJECT_REGISTRY_H
#define TIVIDY_PROJECT_REGISTRY_H

#include <string>
#include <vector>
#include <map>

// Forward declarations
class Project;
class ProjectSession;
class CommandHistory;
class AuditLog;
class Task;
class WorkflowComponent;

/**
 * @brief Manages isolated running project sessions, their history stacks, and audit logs.
 *
 * Acts as the centralised storage container for active project instances, ensuring
 * that each project session maintains independent command histories and state isolation.
 */
class ProjectRegistry
{
private:
    std::map<std::string, ProjectSession *> sessions; ///< Map linking project IDs to their isolated sessions.

public:
    /**
     * @brief Destroys the registry and cleans up all active project sessions.
     */
    ~ProjectRegistry();

    /**
     * @brief Registers a new project instance and wraps it in an isolated session.
     *
     * @details Allocates a `ProjectSession` containing a dedicated `CommandHistory` and
     *          `AuditLog` for the project.
     *
     * @param p Raw pointer to the project instance to register (registry takes ownership).
     * @return `std::string` The unique identifier assigned or extracted from the project.
     * @throws `WorkflowError` If registration fails.
     */
    std::string add(Project *p);

    /**
     * @brief Locates and returns a reference to a project by its identifier.
     *
     * @param id The unique project identifier.
     * @return `Project&` Reference to the target project object.
     * @throws `NotFoundError` If the project ID does not exist in the registry.
     */
    Project &findProject(const std::string &id) const;

    /**
     * @brief Locates and returns a reference to a specific task within a project.
     *
     * @param projectId The unique identifier of the project.
     * @param taskId The unique identifier of the task.
     * @return `Task&` Reference to the target task object.
     * @throws `NotFoundError` If the project or task cannot be located.
     */
    Task &findTask(const std::string &projectId, const std::string &taskId) const;

    /**
     * @brief Locates and returns a reference to any workflow component within a project.
     *
     * @param projectId The unique identifier of the project.
     * @param componentId The unique identifier of the workflow component (Task, DecisionTask, or Project).
     * @return `WorkflowComponent&` Reference to the target component.
     * @throws `NotFoundError` If the project or component cannot be located.
     */
    WorkflowComponent &findComponent(const std::string &projectId, const std::string &componentId) const;

    /**
     * @brief Retrieves the command history manager for a specific project session.
     *
     * @param projectId The unique identifier of the project.
     * @return `CommandHistory&` Reference to the project's dedicated command history stack.
     * @throws `NotFoundError` If the project ID cannot be found.
     */
    CommandHistory &historyOf(const std::string &projectId);

    /**
     * @brief Retrieves the immutable audit log for a specific project session.
     *
     * @param projectId The unique identifier of the project.
     * @return `const AuditLog&` Const reference to the project's audit log.
     * @throws `NotFoundError` If the project ID cannot be found.
     */
    const AuditLog &auditOf(const std::string &projectId) const;

    /**
     * @brief Lists all registered project identifiers.
     *
     * @return `std::vector<std::string>` A collection of all active project IDs.
     */
    std::vector<std::string> listIds() const;
};

#endif // TIVIDY_PROJECT_REGISTRY_H