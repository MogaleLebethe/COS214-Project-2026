#ifndef TIVIDY_COMMAND_H
#define TIVIDY_COMMAND_H

#include <string>
#include <map>
#include <vector>
#include <functional>
#include <nlohmann/json.hpp>

// Forward declarations
class Task;
class DecisionTask;
class Project;
class WorkflowComponent;
class TaskState;
class TaskMemento;

/**
 * @brief Abstract base class representing an executable and reversible command in the command pattern.
 */
class Command {
public:
    virtual ~Command();

    /**
     * @brief Executes the command action.
     */
    virtual void execute() = 0;

    /**
     * @brief Reverses the command action.
     */
    virtual void undo() = 0;

    /**
     * @brief Generates a descriptive string summary of the command.
     */
    virtual std::string describe() const = 0;
};

/**
 * @brief Concrete command for encapsulating task state transition events.
 */
class TaskStateCommand : public Command {
private:
    Task* task;                                     ///< Raw pointer to the target task.
    std::function<void(Task&)> transition;          ///< Transition action function wrapper.
    std::string label;                              ///< Descriptive command label.
    TaskState* previousState;                       ///< Flyweight pointer to the previous task state.

public:
    TaskStateCommand(Task* task, std::function<void(Task&)> transition, const std::string& label);

    void execute() override;
    void undo() override;
    std::string describe() const override;
};

/**
 * @brief Concrete command for adding child work items to a project container.
 */
class AddWorkItemCommand : public Command {
private:
    Project* parent;                                ///< Raw pointer to the parent project container.
    WorkflowComponent* child;                       ///< Raw pointer to the child work item.
    bool applied;                                   ///< Flag tracking whether the command has been successfully applied.

public:
    AddWorkItemCommand(Project* parent, WorkflowComponent* child);
    virtual ~AddWorkItemCommand();

    void execute() override;
    void undo() override;
    std::string describe() const override;
};

/**
 * @brief Concrete command for managing dependency prerequisite links between components.
 */
class AddDependencyCommand : public Command {
private:
    WorkflowComponent* dependent;                   ///< Raw pointer to the dependent component.
    WorkflowComponent* prerequisite;                ///< Raw pointer to the prerequisite component.
    bool added;                                     ///< Flag tracking whether the dependency link was successfully established.

public:
    AddDependencyCommand(WorkflowComponent* dependent, WorkflowComponent* prerequisite);

    void execute() override;
    void undo() override;
    std::string describe() const override;
};

/**
 * @brief Concrete command acting as a memento caretaker for safe task information updates and edits.
 */
class EditTaskInfoCommand : public Command {
private:
    Task* task;                                     ///< Raw pointer to the target task.
    nlohmann::json payload;                         ///< JSON payload containing update fields.
    TaskMemento* oldMemento;                        ///< Raw pointer to the pre-edit snapshot memento.
    TaskMemento* currentMemento;                    ///< Raw pointer to the post-edit snapshot memento.
    bool hasSnapshots;                              ///< Flag indicating if snapshots have been cached.

    /**
     * @brief Parses and applies the JSON modification payload to the task.
     */
    void applyPayload();

public:
    EditTaskInfoCommand(Task* task, const nlohmann::json& payload);
    virtual ~EditTaskInfoCommand();

    void execute() override;
    void undo() override;
    std::string describe() const override;
};

/**
 * @brief Concrete command for handling complex routing decisions and branch exclusions.
 */
class DecideCommand : public Command {
private:
    DecisionTask* task;                             ///< Raw pointer to the decision task.
    nlohmann::json context;                         ///< Evaluation context JSON object.
    std::string chosen;                             ///< Label string of the selected branch.
    std::map<Task*, TaskState*> previousStates;     ///< Map preserving prior states of affected tasks for rollback.

    /**
     * @brief Restores previous states and clears decision metadata upon undo/failure.
     */
    void restore();

public:
    DecideCommand(DecisionTask* task, const nlohmann::json& context);

    void execute() override;
    void undo() override;
    std::string describe() const override;
};

#endif // TIVIDY_COMMAND_H