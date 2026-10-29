#pragma once
#include "TaskState.h"

class PendingState : public TaskState{
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
        PendingState(const PendingState&) = delete;
        PendingState& operator=(const PendingState&) = delete; 

    private:    
        PendingState() {}

}; 