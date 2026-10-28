#include "TaskCompletionCommand.h"

#include <stdexcept>
#include "Task.h"
#include "TaskState.h"

TaskCompletionCommand::TaskCompletionCommand(Task* task)
    : task(task), previousState(nullptr) {
    if (!this->task) {
        throw std::invalid_argument("TaskCompletionCommand: task cannot be null");
    }
}

TaskCompletionCommand::~TaskCompletionCommand() {
    delete previousState; // we own the clone (deleting nullptr is safe)
}

void TaskCompletionCommand::execute() {
    // Already completed: complete() would be a no-op, so record nothing.
    // This stops undo() from "restoring" a state this command never changed.
    if (task->isCompleted()) {
        delete previousState;
        previousState = nullptr;
        return;
    }

    // Snapshot the current state BEFORE completing. Clone it, because
    // Task::setState() will delete the live state object during complete().
    TaskState* snapshot = task->getState()->clone();

    // complete() may throw (e.g. completing a Blocked task). If it does the
    // task is unchanged, so discard the snapshot and record nothing.
    try {
        task->complete();
    } catch (...) {
        delete snapshot;
        throw;
    }

    delete previousState; // drop any older snapshot from a previous run
    previousState = snapshot;
}

void TaskCompletionCommand::undo() {
    // Nothing to undo: execute() never ran, failed, or was a no-op.
    if (!previousState) {
        return;
    }

    // Task::setState() takes ownership of the state it is given and deletes
    // the old one, so hand the clone over and forget it ourselves.
    task->setState(previousState);
    previousState = nullptr;

    // "Adjust dependent statuses": let dependents re-evaluate.
    // NOTE: as specced, evaluateDependencies() only unblocks, it never
    // re-blocks, so dependents already unblocked by the completion stay
    // unblocked. Add a re-block path here if your design needs one.
    task->notifyDependents();
}

void TaskCompletionCommand::redo() {
    // Re-running execute() re-captures previousState, so the command can be
    // undone again afterwards.
    execute();
}
