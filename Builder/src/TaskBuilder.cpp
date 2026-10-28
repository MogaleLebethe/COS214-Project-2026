#include "TaskBuilder.h"
#include "Task.h"
#include "WorkflowComponent.h"
#include "Participant.h"
#include "DateTime.h"
#include "WorkflowError.h"
#include <stdexcept>

TaskBuilder::TaskBuilder() :product(nullptr) {
    reset();
}

TaskBuilder::~TaskBuilder() {
    delete product;
}

void TaskBuilder::reset() {
    delete product;
    product = nullptr;
    product = new Task();
}

void TaskBuilder::setBasicInfo(const std::string&id, const std::string&name) {
    if (!id.empty()) {
        product->id_ = id;
    }
    product->name_ = name;
}

void TaskBuilder::setDescription(const std::string&description) {
    product->description_ =description;
}

void TaskBuilder::setExecutor(const Participant&executor) {
    product->executor_ = executor;   
}

void TaskBuilder::setDeadline(const DateTime&deadline) {
    product->deadline_=deadline;   
}

void TaskBuilder::addDependency(WorkflowComponent*component) {
    product->addDependency(component);
}

void TaskBuilder::addCustomField(const std::string& key, const std::string&value) {
    product->setCustomField(key,value);
}

void TaskBuilder::addWorkItem(WorkflowComponent*) {
    throw std::logic_error("Tasks cannot contain nested workitems");
}

void TaskBuilder::validate() const {
    if (product->getName().empty()) {
        throw WorkflowError("name is mandatory");
    }
    if (product->getId().empty()) {
        throw WorkflowError("id is mandatory");
    }
}

Task* TaskBuilder::getResult() {
    validate();
    Task*finished =product;
    product=nullptr;
    reset();
    return finished;
}

WorkflowComponent* TaskBuilder::build() {
    return getResult();
}