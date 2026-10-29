#include "BlockedState.h"
#include "InProgressState.h"
#include "CancelledState.h"
#include "Task.h"
#include "WorkflowError.h"
#include "FailedState.h"

TaskState* BlockedState::instance(){
    static BlockedState s; 
    return &s; 
}

void BlockedState::start(Task* task){
    task->setState(InProgressState::instance());
}

void BlockedState::complete(Task* task){
    throw InvalidTransitionError("BlockedState::complete: task is blocked until resume");
}

void BlockedState::block(Task* task){
    throw InvalidTransitionError("BlockedState::block: task is already blocked");
}

void BlockedState::fail(Task* task){
    task->setState(FailedState::instance());
}

void BlockedState::cancel(Task* task){
    task->setState(CancelledState::instance());
}

void BlockedState::skip(Task* task){
    throw InvalidTransitionError("BlockedState::skip: not a valid event from blocked");
}

std::string BlockedState::toString() const{
    return "Blocked";
}

bool BlockedState::isSatisfied() const{
    return false; 
}