#pragma once
#include <string>
#include <map>
#include "nlohmann/json.hpp"

class ComponentBuilder;
class Task;
class Project;
class WorkflowComponent;
class RouterRegistry;

class Director {
public:
    Director();

    void registerBuilder(const std::string&type, ComponentBuilder*builder);
    Task* createNewTask(const std::string& name, const std::string& description = "");
    Project* createNewProject(const std::string& name, const std::string& description = "");
    WorkflowComponent*createFromTemplate(const nlohmann::json& templateJson, const RouterRegistry& routers);

private:
    std::map<std::string, ComponentBuilder*> builders_;

    WorkflowComponent*runStandardRecipe(const std::string&type, const std::string& name, const std::string& description);
    WorkflowComponent* buildNode(const nlohmann::json& nodeJson, const RouterRegistry& routers, int depth, std::map<std::string, WorkflowComponent*>& byName);
};
