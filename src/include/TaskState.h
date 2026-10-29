#pragma once 
#include <string>

class Task; 

class TaskState {
    public: 
        virtual ~TaskState();
        virtual void start(Task* task) = 0; 
        virtual void complete(Task* task) =0;
        virtual void block(Task* task) =0; 
        virtual void fail(Task* task) =0; 
        virtual void cancel(Task* task) =0; 
        virtual void skip(Task* task) =0; 
        virtual std::string toString() const =0;
        virtual bool isSatisfied() const =0;  

}; 