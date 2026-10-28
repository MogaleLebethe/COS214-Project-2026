#include "TaskBuilder.h"
#include "Task.h"
#include "WorkflowComponent.h"
#include <stdexcept>

TaskBuilder::TaskBuilder() : product_(new Task()) {}

TaskBuilder::~TaskBuilder() {
.
    delete product_;
}

void TaskBuilder::reset() {
    delete product_;
    product_=new Task;
}

void TaskBuilder::setBasicInfo(const std::string&id,const std::string&name) {
    if(!id.empty()){
        prodcut_->id_=id;
    }
    product_->name_=name;
}
void TaskBuilder::setExecutor(Participant*executor) {
    product_->executor_ =executor;
}

void TaskBuilder::setDeadline(DateTime*deadline) {
    product_->deadline_=deadline;
}

void TaskBuilder::addDependency(WorkflowComponent * component) {
    product_->addDependency(component);
}

void TaskBuilder::setCustomField(const std::string& key, const std::string&value) {
    product_->setCustomField(key,value);
}

void TaskBuilder::addWorkItem(Task*) {
    throw std::logic_error("Tasks can't contain nested workitems");
}

Task* TaskBuilder::getResult() {
    Task* result = product_;
    product_= nullptr;
    reset();
    return result;
}
