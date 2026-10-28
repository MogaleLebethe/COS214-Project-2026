include "ProjectBuilder.h"
#include "Project.h"
#include "Task.h"
#include "WorkflowComponent.h"

ProjectBuilder::ProjectBuilder() : product_(new Project()) {

}

ProjectBuilder::~ProjectBuilder() {
    delete product_;
}

void ProjectBuilder::reset() {
    delete product_;
    product_ =new Project();
}

void ProjectBuilder::setBasicInfo(const std::string&id, const std::string&name) {
    if (!id.empty()) {
        product_->id_=id;
    }
    product_->name_=name;
}

void ProjectBuilder::setDescription(const std::string&description) {
    product_->description_=description;
}

void ProjectBuilder::setExecutor(Participant*executor) {
    product_->executor_=executor;
}

void ProjectBuilder::setDeadline(DateTime * deadline) {
    product_->deadline_ =deadline;
}

void ProjectBuilder::addDependency(WorkflowComponent*component) {
    product_->addDependency(component);
}

void ProjectBuilder::setCustomField(const std::string&key,const std::string&value) {
    product_->setCustomField(key,value);
}

void ProjectBuilder::addWorkItem(Task* task) {
    product_->addWorkItem(task);
}

Project* ProjectBuilder::getResult() {
    Project* result = product_;
    product_ = nullptr;
    reset();
    return result;
}