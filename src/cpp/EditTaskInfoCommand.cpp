#include "EditTaskInfoCommand.h"
#include "Task.h"
#include "Participant.h"
#include "DateTime.h"
#include "WorkflowError.h"

EditTaskInfoCommand::EditTaskInfoCommand(Task* task, const nlohmann::json& payload)
    : task(task), payload(payload), hasSnapshots(false) {}

void EditTaskInfoCommand::execute() {
    if (hasSnapshots) {
        task->restoreMemento(currentMemento);
        return;
    }
    oldMemento = task->createMemento();
    try {
        applyPayload();
    } catch (const nlohmann::json::exception& e) {
        task->restoreMemento(oldMemento);
        throw WorkflowError(std::string("Invalid edit payload: ") + e.what());
    } catch (...) {
        task->restoreMemento(oldMemento);
        throw;
    }
    currentMemento = task->createMemento();
    hasSnapshots = true;
}

void EditTaskInfoCommand::undo() {
    task->restoreMemento(oldMemento);
}

std::string EditTaskInfoCommand::describe() const {
    return "Edit task " + task->getId();
}

void EditTaskInfoCommand::applyPayload() {
    if (!payload.is_object()) throw WorkflowError("Edit payload must be a JSON object");

    for (nlohmann::json::const_iterator it = payload.begin(); it != payload.end(); ++it) {
        const std::string& key = it.key();
        const nlohmann::json& value = it.value();

        if (key == "name") {
            task->setName(value.get<std::string>());
        } else if (key == "description") {
            task->setDescription(value.get<std::string>());
        } else if (key == "executor") {
            task->setExecutor(Participant(value.at("id").get<std::string>(),
                                          value.at("name").get<std::string>(),
                                          value.value("role", std::string())));
        } else if (key == "deadline") {
            task->setDeadline(DateTime(value.at("year").get<int>(), value.at("month").get<int>(),
                                       value.at("day").get<int>(), value.value("hour", 0),
                                       value.value("min", 0)));
        } else if (key == "customFields") {
            for (nlohmann::json::const_iterator f = value.begin(); f != value.end(); ++f)
                task->setCustomField(f.key(), f.value().get<std::string>());
        } else {
            throw WorkflowError("Unknown edit field: " + key);
        }
    }
}
