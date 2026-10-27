#pragma once 
#include <string>

class Task; 

class TaskState {
    public: 
        virtual ~TaskState();
        virtual void start(Task* context) = 0; 
        virtual void complete(Task* context) =0;
        virtual void block(Task* context) =0; 
        virtual std::string toString() const =0; 

}; 