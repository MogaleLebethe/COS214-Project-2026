#include "CancelledState.h"
#include "Task.h"
#include "WorkflowError.h"

TaskState* CancelledState::instance(){
    static CancelledState s; 
    return &s; 
}

void CancelledState::start(Task* task){
    throw InvalidTransitionError("CancelledState::start: Cancelled is terminal state");
}

void CancelledState::complete(Task* task){
    throw InvalidTransitionError("CancelledState::complete:Cancelled is terminal state");
}

void CancelledState::block(Task* task){
    throw InvalidTransitionError("CancelledState::block: Cancelled is terminal state");
}

void CancelledState::fail(Task* task){
    throw InvalidTransitionError("CancelledState::fail: Cancelled is terminal state");
}

void CancelledState::cancel(Task* task){
    throw InvalidTransitionError("CancelledState::cancel: Cancelled is terminal state");
}

void CancelledState::skip(Task* task){
    throw InvalidTransitionError("CancelledState::skip: Cancelled is terminal state");
}

std::string CancelledState::toString() const{
    return "Cancelled";
}

bool CancelledState::isSatisfied() const{
    return false; 
}