#pragma once

#include "Command.h"
#include "TaskMemento.h"
#include "nlohmann/json.hpp"
#include <string>

class Task;

class EditTaskInfoCommand : public Command {
private:
    Task* task;                     // BORROWED
    nlohmann::json payload;
    TaskMemento oldMemento;
    TaskMemento currentMemento;
    bool hasSnapshots;

    void applyPayload();

public:
    // Payload keys: name, description, executor{id,name,role},
    // deadline{year,month,day,hour,min}, customFields{key:value}
    EditTaskInfoCommand(Task* task, const nlohmann::json& payload);

    void execute() override;
    void undo() override;
    std::string describe() const override;
};
