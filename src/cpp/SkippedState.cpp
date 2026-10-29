#include "SkippedState.h"
#include "Task.h"
#include "WorkflowError.h"

TaskState* SkippedState::instance(){
    static SkippedState s; 
    return &s; 
}

void SkippedState::start(Task* task){
    throw InvalidTransitionError("SkippedState::start: Skipped is terminal state");
}

void SkippedState::complete(Task* task){
    throw InvalidTransitionError("SkippedState::complete:Skipped is terminal state");
}

void SkippedState::block(Task* task){
    throw InvalidTransitionError("SkippedState::block: Skipped is terminal state");
}

void SkippedState::fail(Task* task){
    throw InvalidTransitionError("SkippedState::fail: Skipped is terminal state");
}

void SkippedState::cancel(Task* task){
    throw InvalidTransitionError("SkippedState::cancel: Skipped is terminal state");
}

void SkippedState::skip(Task* task){
    throw InvalidTransitionError("SkippedState::skip: Skipped is terminal state");
}

std::string SkippedState::toString() const{
    return "Skipped";
}

bool SkippedState::isSatisfied() const{
    return true; 
}