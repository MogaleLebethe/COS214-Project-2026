#include "Director.h"
#include "ComponentBuilder.h"
#include "TaskBuilder.h"
#include "ProjectBuilder.h"
#include <stdexcept>

Director::Director() : builder_(nullptr) {
    
}

void Director::setBuilder(ComponentBuilder*builder) {
    builder_=builder;
}

Task* Director::buildStandardTask(ComponentBuilder* builder, const std::string&name, const std::string&description) {
    if (builder ==nullptr) {
        throw std::invalid_argument("builder must not be null!");
    }

    builder->reset();
    builder->setBasicInfo("",name);
    builder->setDescription(description);

    TaskBuilder* taskBuilder = dynamic_cast<TaskBuilder*>(builder);
    if (taskBuilder == nullptr) {
        throw std::logic_error("Director::buildStandardTask requires a TaskBuilder");
    }

    return taskBuilder->getResult();
}

Task* Director::createTask() {
    return buildStandardTask(builder_,"New Task","");
}

Project* Director::createProject() {
    if (builder_==nullptr) {
        throw std::invalid_argument("Director::createProject:no builder has been set");
    }

    builder_->reset();
    builder_->setBasicInfo("", "New Project");
    builder_->setDescription("");

    // The get declared on the concrete builders, not on ComponentBuilder,so downcast to confirm ProjectBuilder before using it.
    ProjectBuilder* projectBuilder = dynamic_cast<ProjectBuilder*>(builder_);
    if (projectBuilder ==nullptr) {
        throw std::logic_error("Director::createProject requires a ProjectBuilder");
    }
    return projectBuilder->getResult();
}