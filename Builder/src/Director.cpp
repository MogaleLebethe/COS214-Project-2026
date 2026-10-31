#include "Director.h"
#include "ComponentBuilder.h"
#include "Task.h"
#include "Project.h"
#include "WorkflowComponent.h"
#include <stdexcept>

Director::Director() {}

void Director::registerBuilder(const std::string&type, ComponentBuilder*builder) {
    if (builder==nullptr) {
        throw std::invalid_argument("Director::registerBuilder: builder must not be null");
    }
    builders_[type] = builder;
}

WorkflowComponent* Director::runStandardRecipe(const std::string&type, const std::string&name, const std::string&description) {
    std::map<std::string, ComponentBuilder*>::iterator found=builders_.find(type);
    if (found==builders_.end() ||found->second == nullptr) {
        throw std::logic_error("Director:no builder registered for \"" + type + "\"");
    }
    ComponentBuilder*builder =found->second;
    builder->reset();
    builder->setBasicInfo("", name);
    builder->setDescription(description);
    return builder->build();
}

Task* Director::createNewTask(const std::string& name,const std::string&description) {
    WorkflowComponent*result=runStandardRecipe("Task", name,description);
    Task* task = dynamic_cast<Task*>(result);
    if (task==nullptr) {
        throw std::logic_error("Director::createNewTask: builder registered under \"Task\" did not produce a Task");
    }

    return task;
}

Project* Director::createNewProject(const std::string& name, const std::string&description) {
    WorkflowComponent* result = runStandardRecipe("Project", name,description);
    Project* project = dynamic_cast<Project*>(result);
    if (project==nullptr) {
        throw std::logic_error("Director::createNewProject: builder registered under \"Project\" did not produce a Project");
    }

    
    return project;
}