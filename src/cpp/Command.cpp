/**
 * @file Command.cpp
 * @brief Implementations of Command and the five concrete commands.
 */
#include "Command.h"
#include "DateTime.h"
#include "DecisionTask.h"
#include "Participant.h"
#include "Project.h"
#include "Task.h"
#include "TaskState.h"
#include "WorkflowComponent.h"
#include "WorkflowError.h"

#include <vector>

Command::~Command() {}

// TaskStateCommand

TaskStateCommand::TaskStateCommand(Task* task, std::function<void(Task&)> transition, const std::string& label)
    : task(task), transition(transition), label(label), previousState(nullptr) {}

void TaskStateCommand::execute() {
    previousState = task->getState();
    transition(*task);
}

void TaskStateCommand::undo() {
    task->setState(previousState);
}

std::string TaskStateCommand::describe() const {
    return label + " task " + task->getId();
}

// AddWorkItemCommand

AddWorkItemCommand::AddWorkItemCommand(Project* parent, WorkflowComponent* child)
    : parent(parent), child(child), applied(false) {}

AddWorkItemCommand::~AddWorkItemCommand() {
    if (!applied) delete child;
}

void AddWorkItemCommand::execute() {
    parent->addWorkItem(child);
    applied = true;
}

void AddWorkItemCommand::undo() {
    parent->removeWorkItem(child);
    applied = false;
}

std::string AddWorkItemCommand::describe() const {
    return "Add work item " + child->getName();
}

// AddDependencyCommand

AddDependencyCommand::AddDependencyCommand(WorkflowComponent* dependent, WorkflowComponent* prerequisite)
    : dependent(dependent), prerequisite(prerequisite), added(false) {}

void AddDependencyCommand::execute() {
    added = dependent->addDependency(prerequisite);
}

void AddDependencyCommand::undo() {
    if (added) dependent->removeDependency(prerequisite);
}

std::string AddDependencyCommand::describe() const {
    return "Add dependency " + dependent->getId() + " -> " + prerequisite->getId();
}

// EditTaskInfoCommand

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

// DecideCommand

DecideCommand::DecideCommand(DecisionTask* task, const nlohmann::json& context)
    : task(task), context(context) {}

void DecideCommand::execute() {
    previousSelection = task->getSelectedBranch();
    previousStates.clear();
    previousStates[task] = task->getState();

    std::vector<Task*> targets = task->getBranchTargets();
    for (std::size_t i = 0; i < targets.size(); ++i)
        previousStates[targets[i]] = targets[i]->getState();

    try {
        std::string label = task->decide(context);
        const std::map<std::string, std::vector<Task*> >& branches = task->getBranches();
        const std::vector<Task*>& selected = branches.at(label);

        for (std::map<std::string, std::vector<Task*> >::const_iterator b = branches.begin(); b != branches.end(); ++b) {
            if (b->first == label) continue;
            for (std::size_t i = 0; i < b->second.size(); ++i) {
                Task* t = b->second[i];
                bool alsoSelected = false;
                for (std::size_t j = 0; j < selected.size() && !alsoSelected; ++j)
                    alsoSelected = (selected[j] == t);
                if (!alsoSelected && !t->isCompleted()) t->skip();
            }
        }
        task->complete();
        chosen = label;
    } catch (...) {
        restore();
        throw;
    }
}

void DecideCommand::undo() {
    restore();
}

std::string DecideCommand::describe() const {
    std::string text = "Decide " + task->getId();
    return chosen.empty() ? text : text + " -> " + chosen;
}

void DecideCommand::restore() {
    for (std::map<Task*, TaskState*>::iterator it = previousStates.begin(); it != previousStates.end(); ++it)
        it->first->setState(it->second);

    if (previousSelection.empty()) task->clearDecision();
    else task->setSelectedBranch(previousSelection);
}
