#include "FailedState.h"
#include "InProgressState.h"
#include "CancelledState.h"
#include "Task.h"
#include "WorkflowError.h"

TaskState* FailedState::instance(){
    static FailedState s; 
    return &s; 
}

void FailedState::start(Task* task){
    task->setState(InProgressState::instance());
}

void FailedState::complete(Task* task){
    throw InvalidTransitionError("FailedState::complete:failed task cant completed");
}

void FailedState::block(Task* task){
    throw InvalidTransitionError("FailedState::block: not a valid event from Failed");
}

void FailedState::fail(Task* task){
    throw InvalidTransitionError("FailedState::fail: task already failed");
}

void FailedState::cancel(Task* task){
    task->setState(CancelledState::instance());
}

void FailedState::skip(Task* task){
    throw InvalidTransitionError("FailedState::skip: not valid event for Failed");
}

std::string FailedState::toString() const{
    return "Failed";
}

bool FailedState::isSatisfied() const{
    return false; 
}