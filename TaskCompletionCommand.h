#ifndef TASKCOMPLETIONCOMMAND_H
#define TASKCOMPLETIONCOMMAND_H

#include "Command.h"

class Task;       // Receiver
class TaskState;  // type of the cached snapshot

/**
 * Concrete Command: completes a Task and can undo/redo that completion.
 *
 * UML attributes: task : Task*, previousState : TaskState*
 *
 * Ownership note: previousState holds an OWNED CLONE of the task's state,
 * never the live state object. Task::setState() deletes the old state, so a
 * plain alias would dangle as soon as complete() ran.
 */
class TaskCompletionCommand : public Command {
public:
    explicit TaskCompletionCommand(Task* task);
    ~TaskCompletionCommand() override;

    // The command owns a state snapshot, so it must not be copied.
    TaskCompletionCommand(const TaskCompletionCommand&) = delete;
    TaskCompletionCommand& operator=(const TaskCompletionCommand&) = delete;

    void execute() override;
    void undo() override;
    void redo() override;

private:
    Task* task;               // Receiver (not owned)
    TaskState* previousState; // owned clone, or nullptr when nothing to undo
};

#endif // TASKCOMPLETIONCOMMAND_H
