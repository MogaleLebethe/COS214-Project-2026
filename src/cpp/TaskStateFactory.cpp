#include "TaskStateFactory.h"
#include "TaskState.h"
#include "PendingState.h"
#include "InProgressState.h"
#include "BlockedState.h"
#include "CompletedState.h"
#include "FailedState.h"
#include "CancelledState.h"
#include "SkippedState.h"
#include "WorkflowError.h"

TaskState* TaskStateFactory::get(const std::string& name){
    if (name == PendingState::instance()->toString()){
        return PendingState::instance();
    }
    if (name == InProgressState::instance()->toString()){
        return InProgressState::instance();
    }
    if (name == BlockedState::instance()->toString()){
        return BlockedState::instance();
    }
    if (name == CompletedState::instance()->toString()){
        return CompletedState::instance();
    }
    if (name == FailedState::instance()->toString()){
        return FailedState::instance();
    }
    if (name == CancelledState::instance()->toString()){
        return CancelledState::instance();
    }
    if (name == SkippedState::instance()->toString()){
        return SkippedState::instance();
    }
    throw NotFoundError("TaskStateFactory::get: no state named \"" + name + ""\""); 
    
    

}