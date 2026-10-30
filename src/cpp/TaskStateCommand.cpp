#include "TaskStateCommand.h"
#include "Task.h"
#include "TaskState.h"

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
