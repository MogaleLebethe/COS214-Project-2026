#pragma once

#include <vector>

class Command;
class AuditLog;

class CommandHistory {
private:
    std::vector<Command*> undoStack;    // OWNS every element
    std::vector<Command*> redoStack;    // OWNS every element
    AuditLog* audit;                    // BORROWED (same ProjectSession)

    CommandHistory(const CommandHistory&) = delete;
    CommandHistory& operator=(const CommandHistory&) = delete;

    static void destroyAll(std::vector<Command*>& v);

public:
    explicit CommandHistory(AuditLog* audit);
    ~CommandHistory();

    // TAKES ownership of cmd the moment it is called; deletes it if execute() throws
    void run(Command* cmd);
    void undo();
    void redo();
    bool canUndo() const;
    bool canRedo() const;
};
