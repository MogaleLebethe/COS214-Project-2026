#pragma once

#include "Command.h"
#include <string>

class Project;
class WorkflowComponent;

class AddWorkItemCommand : public Command {
private:
    Project* parent;                // BORROWED
    WorkflowComponent* child;       // OWNED while !applied, otherwise owned by *parent
    bool applied;

public:
    // TAKES ownership of child
    AddWorkItemCommand(Project* parent, WorkflowComponent* child);
    ~AddWorkItemCommand() override;

    void execute() override;
    void undo() override;
    std::string describe() const override;
};
