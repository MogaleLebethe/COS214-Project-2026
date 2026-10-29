#include "PendingState.h"
#include "InProgressState.h"
#include "BlockedState.h"
#include "CancelledState.h"
#include "SkippedState.h"
#include "Task.h"
#include "WorkflowError.h"

TaskState* PendingState::instance(){
    static PendingState s; 
    return &s; 
}

void PendingState::start(Task* task){
    if (!task->isReady()){
        throw InvalidTransitionError("PendingState::start: Prereq not satisfied");
    }
    task->setState(InProgressState::instance());
}

void PendingState::complete(Task*){
    throw InvalidTransitionError("PendingState::complete: task has not been started yet");
}

void PendingState::block(Task* task){
    task->setState(BlockedState::instance());
}

void PendingState::fail(Task*){
    throw InvalidTransitionError("PendingState::fail: not a valid event from pending");

}

void PendingState::cancel(Task* task){
    task->setState(CancelledState::instance());
}

void PendingState::skip(Task* task){
    task->setState(SkippedState::instance());
}

std::string PendingState::toString() const{
    return "Pending";
}

bool PendingState::isSatisfied() const{
    return false; 
}