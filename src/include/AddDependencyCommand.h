#pragma once

#include "Command.h"
#include <string>

class WorkflowComponent;

class AddDependencyCommand : public Command {
private:
    WorkflowComponent* dependent;       // BORROWED
    WorkflowComponent* prerequisite;    // BORROWED
    bool added;

public:
    AddDependencyCommand(WorkflowComponent* dependent, WorkflowComponent* prerequisite);

    void execute() override;
    void undo() override;
    std::string describe() const override;
};
