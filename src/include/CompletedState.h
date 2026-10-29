#pragma once
#include "TaskState.h"

class CompletedState : public TaskState{
    public:
        static TaskState* instance();
        void start(Task* task) override; 
        void complete(Task* task) override; 
        void block(Task* task) override; 
        void fail(Task* task) override;
        void cancel(Task* task) override;
        void skip(Task* task) override;
        std::string toString() const override; 
        bool isSatisfied() const override; 
        CompletedState(const CompletedState&) = delete;
        CompletedState& operator=(const CompletedState&) = delete; 

    private:    
        CompletedState() {}

}; 