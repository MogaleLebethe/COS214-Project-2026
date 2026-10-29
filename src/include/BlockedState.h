#pragma once
#include "TaskState.h"

class BlockedState : public TaskState{
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
        BlockedState(const BlockedState&) = delete;
        BlockedState& operator=(const BlockedState&) = delete; 

    private:    
        BlockedState() {}

}; 