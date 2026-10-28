#pragma once
#include <string>

class ComponentBuilder;
class Task;
class Project;
class Director {
public:
    Director();

    void setBuilder(ComponentBuilder* builder);

    Task* buildStandardTask(ComponentBuilder* builder, const std::string&name, const std::string&description);


    Task* createTask();

    Project* createProject();

private:
    ComponentBuilder*builder_;
};