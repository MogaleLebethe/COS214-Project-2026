#include "CommandHistory.h"
#include "Command.h"
#include "AuditLog.h"
#include "DateTime.h"
#include "WorkflowError.h"

CommandHistory::CommandHistory(AuditLog* audit) : audit(audit) {}

CommandHistory::~CommandHistory() {
    destroyAll(undoStack);
    destroyAll(redoStack);
}

void CommandHistory::destroyAll(std::vector<Command*>& v) {
    for (std::size_t i = 0; i < v.size(); ++i) delete v[i];
    v.clear();
}

void CommandHistory::run(Command* cmd) {
    try {
        cmd->execute();
    } catch (const WorkflowError& e) {
        audit->record("REJECTED", cmd->describe() + ": " + e.what(), DateTime::now());
        delete cmd;
        throw;
    } catch (...) {
        delete cmd;
        throw;
    }
    destroyAll(redoStack);
    undoStack.push_back(cmd);
    audit->record("EXECUTE", cmd->describe(), DateTime::now());
}

void CommandHistory::undo() {
    if (undoStack.empty()) throw WorkflowError("Nothing to undo");
    Command* cmd = undoStack.back();
    cmd->undo();
    redoStack.push_back(cmd);
    undoStack.pop_back();
    audit->record("UNDO", cmd->describe(), DateTime::now());
}

void CommandHistory::redo() {
    if (redoStack.empty()) throw WorkflowError("Nothing to redo");
    Command* cmd = redoStack.back();
    cmd->execute();
    undoStack.push_back(cmd);
    redoStack.pop_back();
    audit->record("REDO", cmd->describe(), DateTime::now());
}

bool CommandHistory::canUndo() const {
    return !undoStack.empty();
}

bool CommandHistory::canRedo() const {
    return !redoStack.empty();
}
