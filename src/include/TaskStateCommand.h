#pragma once

#include "Command.h"
#include <functional>
#include <string>

class Task;
class TaskState;

class TaskStateCommand : public Command {
private:
    Task* task;                                 // BORROWED
    std::function<void(Task&)> transition;
    std::string label;
    TaskState* previousState;                   // BORROWED flyweight

public:
    TaskStateCommand(Task* task, std::function<void(Task&)> transition, const std::string& label);

    void execute() override;
    void undo() override;
    std::string describe() const override;
};
