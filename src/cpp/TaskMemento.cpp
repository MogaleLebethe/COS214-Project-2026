#include "TaskMemento.h"
#include "Task.h"

TaskMemento::TaskMemento(){
}

TaskMemento::TaskMemento(const Task& task){
    this->name = task.name;
    this->description = task.description;
    this->executorId = task.executorId;
    this->deadline = task.deadline;
    this->customFields = task.customFields;
}

// helper
bool TaskMemento::isEmpty() const{
    return name.empty() && description.empty() && executorId.empty() && 
           deadline.empty() && customFields.empty();
}
