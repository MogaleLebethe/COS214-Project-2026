#include "AddWorkItemCommand.h"
#include "Project.h"
#include "WorkflowComponent.h"

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
