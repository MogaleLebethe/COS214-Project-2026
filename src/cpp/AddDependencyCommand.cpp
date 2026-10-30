#include "AddDependencyCommand.h"
#include "WorkflowComponent.h"

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
