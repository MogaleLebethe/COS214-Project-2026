#pragma once

#include "Command.h"
#include "nlohmann/json.hpp"
#include <map>
#include <string>

class Task;
class TaskState;
class DecisionTask;

class DecideCommand : public Command {
private:
    DecisionTask* task;                         // BORROWED
    nlohmann::json context;
    std::string previousSelection;
    std::string chosen;
    std::map<Task*, TaskState*> previousStates; // BORROWED keys and flyweights

    void restore();

public:
    DecideCommand(DecisionTask* task, const nlohmann::json& context);

    void execute() override;
    void undo() override;
    std::string describe() const override;
};
