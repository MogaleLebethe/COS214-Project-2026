/**
 * @file CommandHistory.h
 * @brief Command pattern invoker: one per project.
 */
#pragma once

#include <vector>

class Command;
class AuditLog;

/**
 * @brief Runs commands and keeps the undo/redo stacks for exactly one project.
 *
 * Every run, undo, redo and rejection is recorded in the project's `AuditLog`.
 * A new action clears the redo stack.
 */
class CommandHistory {
private:
    std::vector<Command*> undoStack;    ///< Executed commands, newest last. OWNS every element.
    std::vector<Command*> redoStack;    ///< Undone commands, newest last. OWNS every element.
    AuditLog* audit;                    ///< The project's audit log. BORROWED (same ProjectSession).

    CommandHistory(const CommandHistory&) = delete;
    CommandHistory& operator=(const CommandHistory&) = delete;

    /**
     * @brief Deletes every command in a stack and empties it.
     *
     * @param v The stack to clear.
     */
    static void destroyAll(std::vector<Command*>& v);

public:
    /**
     * @brief Constructs an empty history.
     *
     * @param audit Raw pointer to the project's audit log (borrowed; must outlive this history).
     */
    explicit CommandHistory(AuditLog* audit);

    /**
     * @brief Deletes every command on both stacks.
     */
    ~CommandHistory();

    /**
     * @brief Executes a command, pushes it onto the undo stack and clears redo.
     *
     * TAKES ownership of cmd the moment it is called; deletes it if `execute()` throws.
     *
     * @param cmd Raw pointer to a newly created command.
     * @throws `WorkflowError` If the action is rejected. Recorded as REJECTED in the audit log.
     */
    void run(Command* cmd);

    /**
     * @brief Undoes the most recent command and moves it to the redo stack.
     *
     * @throws `WorkflowError` If there is nothing to undo.
     */
    void undo();

    /**
     * @brief Re-executes the most recently undone command and moves it back to the undo stack.
     *
     * @throws `WorkflowError` If there is nothing to redo.
     */
    void redo();

    /**
     * @brief Checks whether there is a command to undo.
     *
     * @return `bool` `true` If the undo stack is not empty, `false` otherwise.
     */
    bool canUndo() const;

    /**
     * @brief Checks whether there is a command to redo.
     *
     * @return `bool` `true` If the redo stack is not empty, `false` otherwise.
     */
    bool canRedo() const;
};
