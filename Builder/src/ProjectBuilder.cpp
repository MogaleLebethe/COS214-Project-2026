#include "ProjectBuilder.h"
#include "Project.h"
#include "WorkflowComponent.h"
#include "Participant.h"
#include "DateTime.h"
#include "WorkflowError.h"

ProjectBuilder::ProjectBuilder() : product(nullptr) {
    reset();
}

ProjectBuilder::~ProjectBuilder() {
    delete product;
}

void ProjectBuilder::reset() {
    delete product;
    product = nullptr;
    product = new Project();
}

void ProjectBuilder::setBasicInfo(const std::string&id, const std::string&name) {
    if (!id.empty()) {
        product->id_=id;
    }
    product->name_=name;
}

void ProjectBuilder::setDescription(const std::string&description) {
    product->description_=description;
}

void ProjectBuilder::setExecutor(const Participant&executor) {
    product->executor_= executor;
}

void ProjectBuilder::setDeadline(const DateTime&deadline) {
    product->deadline_= deadline;
}

void ProjectBuilder::addDependency(WorkflowComponent*component) {
    product->addDependency(component);
}

void ProjectBuilder::addCustomField(const std::string&key, const std::string& value) {
    product->setCustomField(key,value);
}

void ProjectBuilder::addWorkItem(WorkflowComponent*item) {
    product->addWorkItem(item);
}

void ProjectBuilder::validate() const {
    if (product->getName().empty()) {
        throw WorkflowError("ProjectBuilder::validate: name is mandatory");
    }
    if (product->getId().empty()) {
        throw WorkflowError("ProjectBuilder::validate: id is mandatory");
    }
}

Project* ProjectBuilder::getResult() {
    validate();
    Project* finished=product;
    product = nullptr;
    reset();
    return finished;
}

WorkflowComponent* ProjectBuilder::build() {
    return getResult();
}