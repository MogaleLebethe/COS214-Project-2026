#ifndef TIVIDY_EXECUTION_H
#define TIVIDY_EXECUTION_H

#include <string>
#include <nlohmann/json.hpp>

/**
 * @brief Interface for executing actions, state changes, and history manipulation.
 * 
 * Exposes runtime controls for mutating task states, triggering routing decisions,
 * patching task metadata, and navigating the undo/redo history stacks.
 */
class Execution {
public:
    /**
     * @brief Virtual destructor for the interface.
     */
    virtual ~Execution() = default;

    /**
     * @brief Performs a registered action on a specific task.
     * 
     * @details Looks up an action creator from the action registry using the action name, 
     *          instantiates the corresponding command, and executes it via the project's history manager.
     * 
     * @param projectId The unique identifier of the project.
     * @param taskId The unique identifier of the target task.
     * @param action The string identifier of the action (e.g., "start", "complete", "block").
     * @param args Optional JSON arguments required by the action command creator.
     * @throws `NotFoundError` If the project, task, or action name is not found.
     * @throws `InvalidTransitionError` If the state transition is illegal.
     */
    virtual void performAction(std::string projectId, std::string taskId, std::string action, const nlohmann::json& args = {}) = 0;

    /**
     * @brief Applies partial updates (patches) to task information.
     * 
     * @details Instantiates an `EditTaskInfoCommand` to capture a memento snapshot of 
     *          current fields before applying the modifications specified in the payload.
     * 
     * @param projectId The unique identifier of the project.
     * @param taskId The unique identifier of the task being edited.
     * @param payload A JSON payload containing the fields and values to update.
     * @throws `NotFoundError` If the project or task cannot be located.
     */
    virtual void editTask(std::string projectId, std::string taskId, const nlohmann::json& payload) = 0;

    /**
     * @brief Triggers a routing decision on a decision task.
     * 
     * @details Executes a `DecideCommand` which evaluates a routing strategy against 
     *          the provided context, selects a branch, and marks unchosen branches as skipped.
     * 
     * @param projectId The unique identifier of the project.
     * @param taskId The unique identifier of the decision task.
     * @param context A JSON context object used by the routing strategy to evaluate branches.
     * @return `std::string` The label of the selected branch route.
     * @throws `NotFoundError` If the project or decision task is not found.
     * @throws `WorkflowError` If the task is not a decision task or lacks a valid router.
     */
    virtual std::string decide(std::string projectId, std::string taskId, const nlohmann::json& context) = 0;

    /**
     * @brief Reverts the last executed command in a project's history stack.
     * 
     * @details Pops the top command from the undo stack, invokes its undo operation, 
     *          and pushes it onto the redo stack.
     * 
     * @param projectId The unique identifier of the target project session.
     * @throws `NotFoundError` If the project ID is invalid.
     * @throws `WorkflowError` If the undo stack is empty.
     */
    virtual void undo(std::string projectId) = 0;

    /**
     * @brief Re-applies the last reverted command in a project's history stack.
     * 
     * @details Pops the top command from the redo stack, re-executes it, and pushes 
     *          it back onto the undo stack.
     * 
     * @param projectId The unique identifier of the target project session.
     * @throws `NotFoundError` If the project ID is invalid.
     * @throws `WorkflowError` If the redo stack is empty.
     */
    virtual void redo(std::string projectId) = 0;

    /**
     * @brief Checks whether an undo operation can be performed on the project.
     * 
     * @param projectId The unique identifier of the target project session.
     * @return `bool` `true` If there are commands available on the undo stack, `false` otherwise.
     * @throws `NotFoundError` If the project ID is invalid.
     */
    virtual bool canUndo(std::string projectId) const = 0;

    /**
     * @brief Checks whether a redo operation can be performed on the project.
     * 
     * @param projectId The unique identifier of the target project session.
     * @return `bool` `true` If there are commands available on the redo stack, `false` otherwise.
     * @throws `NotFoundError` If the project ID is invalid.
     */
    virtual bool canRedo(std::string projectId) const = 0;
};

#endif // TIVIDY_EXECUTION_H