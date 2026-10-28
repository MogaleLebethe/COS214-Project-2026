#ifndef UPDATETASKATTRIBUTESCOMMAND_H
#define UPDATETASKATTRIBUTESCOMMAND_H

#include "Command.h"

class Task;               // Receiver / Memento originator
class WorkflowCaretaker;  // keeps the undo/redo mementos

/**
 * Concrete Command: makes edits to a Task's attributes (name, description,
 * executor, deadline, custom fields) undoable and redoable by delegating to
 * the WorkflowCaretaker's memento stacks.
 *
 * UML constructor: UpdateTaskAttributesCommand(t : Task*, ct : WorkflowCaretaker*)
 *
 * CONTRACT: execute() takes the "before" snapshot via the caretaker, so it
 * must be called BEFORE the task's attributes are changed. If the attributes
 * are changed first, the snapshot captures the new values and undo() has
 * nothing to go back to.
 *
 * Use one WorkflowCaretaker per Task. The caretaker's undo/redo pop the most
 * recent memento regardless of which task it came from, so a caretaker shared
 * between tasks would restore the wrong task's data.
 */
class UpdateTaskAttributesCommand : public Command {
public:
    UpdateTaskAttributesCommand(Task* t, WorkflowCaretaker* ct);

    void execute() override;
    void undo() override;
    void redo() override;

private:
    // NOTE: the UML class currently shows no attributes. These two need to be
    // added to the diagram (task : Task*, caretaker : WorkflowCaretaker*).
    Task* task;                 // not owned
    WorkflowCaretaker* caretaker; // not owned
};

#endif // UPDATETASKATTRIBUTESCOMMAND_H
