#ifndef TIVIDY_PROJECT_REGISTRY_H
#define TIVIDY_PROJECT_REGISTRY_H

#include <string>
#include <vector>
#include <map>

// Forward declarations
class Project;
class Task;
class WorkflowComponent;
class Command;
class CommandHistory;
class AuditLog;
class ProjectSession;

/**
 * @brief Manages active project sessions, isolated project hierarchies, command histories, and audit logs.
 */
class ProjectRegistry {
private:
    std::map<std::string, ProjectSession*> sessions; ///< Map of project IDs to heap-allocated session objects.

public:
    /**
     * @brief Destroys the project registry and releases all managed project sessions.
     */
    virtual ~ProjectRegistry();

    /**
     * @brief Adds a new project to the registry wrapped in a managed session.
     * 
     * @param p Raw pointer to the `Project` to add.
     * @return `std::string` The unique identifier of the added project.
     * @throws `WorkflowError` If the pointer is null or a session with the same ID already exists.
     */
    std::string add(Project* p);

    /**
     * @brief Finds and retrieves a project reference by its unique identifier.
     * 
     * @param id Const reference to the project identifier string.
     * @return `Project&` Reference to the active project instance.
     * @throws `NotFoundError` If the project session is not found.
     * @throws `WorkflowError` If the active project pointer is null.
     */
    Project& findProject(const std::string& id) const;

    /**
     * @brief Finds and retrieves a concrete task reference within a specific project tree.
     * 
     * @param projectId Const reference to the target project identifier string.
     * @param taskId Const reference to the target task identifier string.
     * @return `Task&` Reference to the found task instance.
     * @throws `NotFoundError` If the project or task cannot be found.
     * @throws `WorkflowError` If the found component is not a task.
     */
    Task& findTask(const std::string& projectId, const std::string& taskId) const;

    /**
     * @brief Finds and retrieves any workflow component reference within a specific project tree.
     * 
     * @param projectId Const reference to the target project identifier string.
     * @param componentId Const reference to the target component identifier string.
     * @return `WorkflowComponent&` Reference to the found workflow component.
     * @throws `NotFoundError` If the project or component cannot be found.
     */
    WorkflowComponent& findComponent(const std::string& projectId, const std::string& componentId) const;

    /**
     * @brief Retrieves the command history manager for a specific project session.
     * 
     * @param projectId Const reference to the target project identifier string.
     * @return `CommandHistory&` Reference to the project's command history controller.
     * @throws `NotFoundError` If the project session is not found.
     * @throws `WorkflowError` If the command history is null.
     */
    CommandHistory& historyOf(const std::string& projectId);

    /**
     * @brief Retrieves the immutable audit log for a specific project session.
     * 
     * @param projectId Const reference to the target project identifier string.
     * @return `const AuditLog&` Const reference to the project's audit log.
     * @throws `NotFoundError` If the project session is not found.
     * @throws `WorkflowError` If the audit log is null.
     */
    const AuditLog& auditOf(const std::string& projectId) const;

    /**
     * @brief Executes a command routed through the project session for audit tracking.
     * 
     * @param projectId Const reference to the target project identifier string.
     * @param cmd Raw pointer to the command to execute (ownership transferred).
     * @throws `NotFoundError` If the project session is not found.
     */
    void execute(const std::string& projectId, Command* cmd);

    /**
     * @brief Undoes the last executed command on a project session.
     * 
     * @param projectId Const reference to the target project identifier string.
     * @throws `NotFoundError` If the project session is not found.
     */
    void undo(const std::string& projectId);

    /**
     * @brief Redoes the last undone command on a project session.
     * 
     * @param projectId Const reference to the target project identifier string.
     * @throws `NotFoundError` If the project session is not found.
     */
    void redo(const std::string& projectId);

    /**
     * @brief Retrieves a list of all active project identifier strings in the registry.
     * 
     * @return `std::vector<std::string>` Vector of project ID strings.
     */
    std::vector<std::string> listIds() const;
};

#endif // TIVIDY_PROJECT_REGISTRY_H