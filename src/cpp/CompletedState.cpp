#include "CompletedState.h"
#include "InProgressState.h"
#include "Task.h"
#include "WorkflowError.h"

TaskState* CompletedState::instance(){
    static CompletedState s; 
    return &s; 
}

void CompletedState::start(Task* task){
    task->setState(InProgressState::instance());
}

void CompletedState::complete(Task* task){
    throw InvalidTransitionError("CompletedState::complete: task is already completed");
}

void CompletedState::block(Task* task){
    throw InvalidTransitionError("CompletedState::block: a completed task cannot be blocked");
}

void CompletedState::fail(Task* task){
    throw InvalidTransitionError("CompletedState::fail: a completed task cannot fail");
}

void CompletedState::cancel(Task* task){
    throw InvalidTransitionError("CompletedState::cancel: a complete task cant be cancelled");
}

void CompletedState::skip(Task* task){
    throw InvalidTransitionError("CompletedState::skip: a completed task cant be skipped");
}

std::string CompletedState::toString() const{
    return "Complete";
}

bool CompletedState::isSatisfied() const{
    return true; 
}