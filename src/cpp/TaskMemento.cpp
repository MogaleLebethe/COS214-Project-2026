#include "TaskMemento.h"
#include "Task.h"


TaskMemento::TaskMemento() : hasSnapshot(false), deadline(0,0,0) {
}

TaskMemento::TaskMemento(const Task& task){
    this->name = task.name;
    this->description = task.description;
    this->executorId = task.executorId;
    this->deadline = task.deadline;
    this->participants = task.participants;
    this->customFields = task.customFields;

    hasSnapshot = true; 
}

// helper
bool TaskMemento::isEmpty() const{
    return !hasSnapshot;
}
