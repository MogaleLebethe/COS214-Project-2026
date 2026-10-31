#ifndef TIVIDY_COMMAND_H
#define TIVIDY_COMMAND_H

#include <functional>
#include <map>
#include <string>
#include <nlohmann/json.hpp>

// Forward declarations
class Task;
class DecisionTask;
class Project;
class WorkflowComponent;
class TaskState;
class TaskMemento;

/**
 * @brief Abstract base class representing an executable and reversible command.
 * 
 * Implements the Command design pattern, encapsulating actions and mutations 
 * performed on project states, tasks, and components to support undo/redo history stacks.
 */
class Command {
public:
    /**
     * @virtual ~Command()
     * @brief Virtual destructor for the command interface.
     */
    virtual ~Command() = default;

    /**
     * @brief Executes the command action.
     * 
     * @details Performs the target mutation on the workflow component, task, or project, 
     *          capturing any necessary memento snapshots for subsequent rollback operations.
     * 
     * @throws `WorkflowError` If execution fails due to invalid state or preconditions.
     */
    virtual void execute() = 0;

    /**
     * @brief Reverts the command action, restoring previous state.
     * 
     * @details Uses stored memento snapshots or inverse operations to return the system 
     *          to the exact state it was in before `execute()` was called.
     * 
     * @throws `WorkflowError` If rollback operations fail.
     */
    virtual void undo() = 0;

    /**
     * @brief Generates a textual description of the command for logging and auditing.
     * 
     * @return `std::string` A human-readable description of the command action.
     */
    virtual std::string describe() const = 0;
};

/**
 * @brief Concrete command for mutating task states with undo/redo rollback support.
 */
class TaskStateCommand : public Command {
private:
    Task* task;                                 ///< Non-owning raw pointer to the target task.
    std::string label;                          ///< Label describing the state change operation.
    TaskState* previousState;                   ///< Non-owning raw pointer capturing the state prior to execution.
    std::function<void(Task&)> transition;     ///< Lambda encapsulating the state transition logic.

public:
    /**
     * @brief Constructs a task state mutation command.
     * 
     * @param t Raw pointer to the target task.
     * @param trans Lambda containing the state transition behavior.
     * @param lbl Descriptive string label for the command.
     */
    TaskStateCommand(Task* t, std::function<void(Task&)> trans, std::string lbl);

    void execute() override;
    void undo() override;
    std::string describe() const override;
};

/**
 * @brief Concrete command for adding a new work item component to a parent project container.
 */
class AddWorkItemCommand : public Command {
private:
    Project* parentProject;                     ///< Non-owning raw pointer to the parent project container.
    WorkflowComponent* childComponent;          ///< Raw pointer to the child component being added (command takes ownership).
    bool applied;                               ///< Flag tracking whether the item has been applied.

public:
    /**
     * @brief Constructs an add work item command.
     * 
     * @param parent Raw pointer to the parent project.
     * @param child Raw pointer to the child component to add.
     */
    AddWorkItemCommand(Project* parent, WorkflowComponent* child);

    /**
     * @brief Destroys the command and safely cleans up owned child components if unapplied.
     */
    ~AddWorkItemCommand();

    void execute() override;
    void undo() override;
    std::string describe() const override;
};

/**
 * @brief Concrete command for establishing a prerequisite dependency between workflow components.
 */
class AddDependencyCommand : public Command {
private:
    WorkflowComponent* dependentComponent;      ///< Non-owning raw pointer to the dependent component.
    WorkflowComponent* prerequisiteComponent;   ///< Non-owning raw pointer to the prerequisite component.

public:
    /**
     * @brief Constructs an add dependency command.
     * 
     * @param dependent Raw pointer to the dependent component.
     * @param prerequisite Raw pointer to the prerequisite component.
     */
    AddDependencyCommand(WorkflowComponent* dependent, WorkflowComponent* prerequisite);

    void execute() override;
    void undo() override;
    std::string describe() const override;
};

/**
 * @brief Concrete command for editing task metadata attributes with memento snapshot support.
 */
class EditTaskInfoCommand : public Command {
private:
    Task* task;                                 ///< Non-owning raw pointer to the target task being edited.
    nlohmann::json payload;                     ///< JSON payload containing the updated field attributes.
    TaskMemento* oldMemento;                    ///< Raw pointer to the memento capturing state prior to edits.
    TaskMemento* currentMemento;                ///< Raw pointer to the memento capturing state after edits.

public:
    /**
     * @brief Constructs an edit task info command.
     * 
     * @param t Raw pointer to the target task.
     * @param p JSON payload of field updates.
     */
    EditTaskInfoCommand(Task* t, const nlohmann::json& p);

    /**
     * @brief Destroys the command and cleans up allocated memento snapshots.
     */
    ~EditTaskInfoCommand();

    void execute() override;
    void undo() override;
    std::string describe() const override;
};

/**
 * @brief Concrete command for executing routing decisions on a decision task.
 */
class DecideCommand : public Command {
private:
    DecisionTask* task;                         ///< Non-owning raw pointer to the decision task.
    nlohmann::json context;                     ///< JSON context evaluated by the routing strategy.
    std::string previousSelection;              ///< String label of the previously selected branch route.
    std::map<Task*, TaskState*> previousStates; ///< Map capturing previous states of affected branch targets for rollback.

public:
    /**
     * @brief Constructs a decide command.
     * 
     * @param dt Raw pointer to the target decision task.
     * @param ctx JSON evaluation context.
     */
    DecideCommand(DecisionTask* dt, const nlohmann::json& ctx);

    /**
     * @brief Destroys the decide command and cleans up any stored state maps.
     */
    ~DecideCommand();

    void execute() override;
    void undo() override;
    std::string describe() const override;
};

#endif // TIVIDY_COMMAND_H
