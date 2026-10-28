#ifndef COMMAND_H
#define COMMAND_H

/**
 * Abstract Command (Command pattern), as in the TIVIDY class diagram.
 * Every concrete command can run its action, reverse it, and re-apply it.
 */
class Command {
public:
    virtual ~Command() = default;

    /** Perform the action. */
    virtual void execute() = 0;

    /** Reverse the effect of the last successful execute()/redo(). */
    virtual void undo() = 0;

    /** Re-apply the action after an undo(). */
    virtual void redo() = 0;
};

#endif // COMMAND_H
