#pragma once
#include "TaskState.h"

class CancelledState : public TaskState{
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
        CancelledState(const CancelledState&) = delete;
        CancelledState& operator=(const CancelledState&) = delete; 

    private:    
        CancelledState() {}

}; 