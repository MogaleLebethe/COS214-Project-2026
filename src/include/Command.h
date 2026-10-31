/**
 * @file Command.h
 * @brief Command pattern: abstract Command and the five concrete commands.
 */
#pragma once

#include <functional>
#include <map>
#include <string>

#include "Support.h"
#include "TaskMemento.h"
#include "nlohmann/json.hpp"

class Task;
class TaskState;
class Project;
class WorkflowComponent;
class DecisionTask;

/**
 * @brief Abstract base for every undoable, auditable action in the workflow engine.
 *
 * Each command is owned by exactly one `CommandHistory`. Redo is not a separate
 * operation: `CommandHistory::redo` calls `execute()` again, so every command must be
 * safely re-executable after `undo()`.
 */
class Command {
public:
    /**
     * @brief Default constructor.
     */
    Command() {}

    /**
     * @brief Virtual destructor so concrete commands are deleted correctly through a base pointer.
     */
    virtual ~Command();

    /**
     * @brief Performs the action.
     *
     * @throws `WorkflowError` If the action is not allowed. The system is left unchanged.
     */
    virtual void execute() = 0;

    /**
     * @brief Reverts the effect of the last `execute()`.
     */
    virtual void undo() = 0;

    /**
     * @brief Describes the action for the audit log.
     *
     * @return `std::string` One line of human-readable text.
     */
    virtual std::string describe() const = 0;

private:
    Command(const Command&) = delete;
    Command& operator=(const Command&) = delete;
};

/**
 * @brief Generic lifecycle action (start, complete, block, fail, cancel, or an industry action).
 *
 * Applies a transition through the State pattern. The task's current `TaskState`
 * decides whether the transition is legal, not the command.
 */
class TaskStateCommand : public Command {
private:
    Task* task;                                 ///< Target task. BORROWED.
    std::function<void(Task&)> transition;      ///< Calls the lifecycle event on the task.
    std::string label;                          ///< Action name shown in the audit log.
    TaskState* previousState;                   ///< State before execute(). BORROWED flyweight.

public:
    /**
     * @brief Constructs a lifecycle command.
     *
     * @param task Raw pointer to the target task (borrowed).
     * @param transition Functor that fires the event, e.g. a lambda calling `Task::complete`.
     * @param label Action name used in the audit log (e.g. "complete").
     */
    TaskStateCommand(Task* task, std::function<void(Task&)> transition, const std::string& label);

    /**
     * @brief Remembers the current state, then applies the transition.
     *
     * @throws `InvalidTransitionError` If the current state does not allow the event.
     */
    void execute() override;

    /**
     * @brief Restores the remembered state and re-notifies dependents.
     */
    void undo() override;

    /**
     * @brief Describes the action for the audit log.
     *
     * @return `std::string` e.g. "complete task T-3".
     */
    std::string describe() const override;
};

/**
 * @brief Adds a component (task, decision or nested project) to a project.
 *
 * Ownership flips with the `applied` flag: the command owns the child while the add is
 * undone, and the project owns it once the add is applied.
 */
class AddWorkItemCommand : public Command {
private:
    Project* parent;                ///< Project receiving the child. BORROWED.
    WorkflowComponent* child;       ///< OWNED by this command while !applied, otherwise by *parent.
    bool applied;                   ///< True while the child is inside the project.

public:
    /**
     * @brief Constructs the command. TAKES ownership of child.
     *
     * @param parent Raw pointer to the project to add to (borrowed).
     * @param child Raw pointer to an edge-free component; this command takes ownership.
     */
    AddWorkItemCommand(Project* parent, WorkflowComponent* child);

    /**
     * @brief Deletes the child only if it is not currently owned by the project.
     */
    ~AddWorkItemCommand() override;

    /**
     * @brief Adds the child to the project; ownership moves to the project.
     *
     * @throws `WorkflowError` If the project rejects the child. The command keeps ownership.
     */
    void execute() override;

    /**
     * @brief Removes the child from the project; ownership returns to the command.
     */
    void undo() override;

    /**
     * @brief Describes the action for the audit log.
     *
     * @return `std::string` e.g. "Add work item Review claim".
     */
    std::string describe() const override;
};

/**
 * @brief Makes one component depend on another.
 *
 * Undo only removes a link this command actually created.
 */
class AddDependencyCommand : public Command {
private:
    WorkflowComponent* dependent;       ///< Component that waits. BORROWED.
    WorkflowComponent* prerequisite;    ///< Component that must finish first. BORROWED.
    bool added;                         ///< True if execute() created a new link.

public:
    /**
     * @brief Constructs the command.
     *
     * @param dependent Raw pointer to the component that will wait (borrowed).
     * @param prerequisite Raw pointer to the component that must finish first (borrowed).
     */
    AddDependencyCommand(WorkflowComponent* dependent, WorkflowComponent* prerequisite);

    /**
     * @brief Links the two components in both directions.
     *
     * @throws `CyclicDependencyError` If the link would create a cycle or a self-dependency.
     */
    void execute() override;

    /**
     * @brief Removes the link if this command created it.
     */
    void undo() override;

    /**
     * @brief Describes the action for the audit log.
     *
     * @return `std::string` e.g. "Add dependency T-2 -> T-1".
     */
    std::string describe() const override;
};

/**
 * @brief Edits task attributes. Memento caretaker for `Task`.
 *
 * Supported payload keys: `name`, `description`, `executor` {id, name, role},
 * `deadline` {year, month, day, hour, min} and `customFields` {key: value}.
 * The edit is all-or-nothing: a bad payload leaves the task unchanged.
 */
class EditTaskInfoCommand : public Command {
private:
    Task* task;                     ///< Task being edited. BORROWED.
    nlohmann::json payload;         ///< Requested changes.
    TaskMemento oldMemento;         ///< Snapshot before the edit (value, no new/delete).
    TaskMemento currentMemento;     ///< Snapshot after the edit, used by redo.
    bool hasSnapshots;              ///< True once the first execute() succeeded.

    /**
     * @brief Applies each payload key to the task.
     *
     * @throws `WorkflowError` If a key is unknown.
     */
    void applyPayload();

public:
    /**
     * @brief Constructs the command.
     *
     * @param task Raw pointer to the task to edit (borrowed).
     * @param payload JSON object of the fields to change.
     */
    EditTaskInfoCommand(Task* task, const nlohmann::json& payload);

    /**
     * @brief Applies the edit, or restores the post-edit snapshot on redo.
     *
     * @throws `WorkflowError` For unknown keys or malformed values. The task is restored first.
     */
    void execute() override;

    /**
     * @brief Restores the pre-edit snapshot.
     */
    void undo() override;

    /**
     * @brief Describes the action for the audit log.
     *
     * @return `std::string` e.g. "Edit task T-3".
     */
    std::string describe() const override;
};

/**
 * @brief Makes a routing decision on a `DecisionTask`.
 *
 * Asks the task's `RoutingStrategy` for a branch, skips the targets of every branch not
 * chosen, then completes the decision task so the chosen targets become ready.
 * A rejected decision changes nothing. A decision is final: to change it, undo it first.
 */
class DecideCommand : public Command {
private:
    DecisionTask* task;                         ///< Decision task. BORROWED.
    nlohmann::json context;                     ///< Data passed to the routing strategy.
    std::string chosen;                         ///< Branch chosen by the last successful execute().
    std::map<Task*, TaskState*> previousStates; ///< States before execute(). BORROWED keys and flyweights.

    /**
     * @brief Restores every recorded state and clears the selection.
     */
    void restore();

public:
    /**
     * @brief Constructs the command.
     *
     * @param task Raw pointer to the decision task (borrowed).
     * @param context JSON data the task's `RoutingStrategy` uses to pick a branch.
     */
    DecideCommand(DecisionTask* task, const nlohmann::json& context);

    /**
     * @brief Decides, skips the other branches and completes the decision task.
     *
     * @throws `NotFoundError` If the router is missing or returns an unknown branch label.
     * @throws `InvalidTransitionError` If the decision task has not been started or a decision was already made.
     */
    void execute() override;

    /**
     * @brief Restores every task's previous state and clears the selection.
     */
    void undo() override;

    /**
     * @brief Describes the action for the audit log.
     *
     * @return `std::string` e.g. "Decide T-4 -> approve".
     */
    std::string describe() const override;
};
