
#include "InProgressState.h"
#include "CompletedState.h"
#include "BlockedState.h"
#include "CancelledState.h"
#include "Task.h"
#include "WorkflowError.h"
#include "FailedState.h"

TaskState* InProgressState::instance(){
    static InProgressState s; 
    return &s; 
}

void InProgressState::start(Task* task){

    throw InvalidTransitionError("InProgressState::start:task is already in progress");
}

void InProgressState::complete(Task* task){
    task->setState(CompletedState::instance());
}

void InProgressState::block(Task* task){
    task->setState(BlockedState::instance());
}

void InProgressState::fail(Task* task){
    task->setState(FailedState::instance());
}

void InProgressState::cancel(Task* task){
    task->setState(CancelledState::instance());
}

void InProgressState::skip(Task* task){
    throw InvalidTransitionError("InProgressState::skip: not a valid event from In Progress");
}

std::string InProgressState::toString() const{
    return "In Progress";
}

bool InProgressState::isSatisfied() const{
    return false; 
}