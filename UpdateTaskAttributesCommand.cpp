#include "UpdateTaskAttributesCommand.h"

#include <stdexcept>
#include "Task.h"
#include "WorkflowCaretaker.h"

UpdateTaskAttributesCommand::UpdateTaskAttributesCommand(Task* t, WorkflowCaretaker* ct)
    : task(t), caretaker(ct) {
    if (!task || !caretaker) {
        throw std::invalid_argument(
            "UpdateTaskAttributesCommand: task and caretaker cannot be null");
    }
}

void UpdateTaskAttributesCommand::execute() {
    // Snapshot the attributes as they are NOW (before the edit).
    // saveSnapshot() also clears the caretaker's redo stack, which is correct:
    // a fresh edit invalidates anything that was previously undone.
    caretaker->saveSnapshot(*task);
}

void UpdateTaskAttributesCommand::undo() {
    // Caretaker pushes the current attributes onto its redo stack, then
    // restores the last saved snapshot into the task.
    if (caretaker->canUndo()) {
        caretaker->undo(*task);
    }
}

void UpdateTaskAttributesCommand::redo() {
    // Caretaker pushes the current attributes onto its undo stack, then
    // restores the most recently undone snapshot into the task.
    if (caretaker->canRedo()) {
        caretaker->redo(*task);
    }
}
