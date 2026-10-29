//Flyweight Factory

#pragma once 
#include <string>

class TaskState;
class TaskStateFactory{
    public:
        static TaskState* get(const std::string& name);
};