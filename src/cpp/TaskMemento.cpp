#include "TaskMemento.h"
#include "Task.h"

TaskMemento::TaskMemento(){
}

TaskMemento::TaskMemento(const Task& task){
    this->name = task.name_;
    this->description = task.description_;
    this->executorId = task.executorId_;
    this->deadline = task.deadline_;
    this->customFields = task.customFields_;
}

// helper
bool TaskMemento::isEmpty() const{
    return name.empty() && description.empty() && executorId.empty() && 
           deadline.empty() && customFields.empty();
}
