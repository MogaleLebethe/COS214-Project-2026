#include "Task.h"
#include "Support.h"
#include "TaskState.h"
#include "TaskMemento.h"
#include "WorkflowError.h"
#include <sstream>

int Task::counter = 0;

Task::Task() : currentState(PendingState::instance()), failedByPrerequisite(false) {
    id = generateId();
}

Task::Task(const Task& other)
    : WorkflowComponent(other),
      currentState(PendingState::instance()),
      failedByPrerequisite(false) {
    id = generateId();
}

Task::~Task() {
}

std::string Task::generateId(){
    std::ostringstream out;
    out << "T-" << ++counter;
    return out.str();
}

void Task::start()  {
    currentState->start(this);
}

void Task::complete() {
    currentState->complete(this);
}

void Task::block() {
    currentState->block(this); 
}

void Task::fail() {
    currentState->fail(this); 
}

void Task::cancel() {
    currentState->cancel(this);
}

void Task::skip() {
    currentState->skip(this);
}

TaskState* Task::getState() const {
    return currentState;
}

void Task::setState(TaskState* state) {
    if (state ==nullptr) {
        throw WorkflowError("Task::setState: state must not be null");
    }
    currentState = state;
    failedByPrerequisite = false; 
    notifyDependents();
}

TaskMemento* Task::createMemento() const {
    TaskMemento* m= nullptr;
    try {
        m = new TaskMemento(*this);
    } catch (...) {
        delete m;
        throw;
    }
    return m;
}

void Task::restoreMemento(const TaskMemento* m){
    if (m == nullptr || m->isEmpty()) {
        throw WorkflowError("Task::restoreMemento: memento is null or empty");
    }
    name = m->name;
    description = m->description;
    executorId = m->executorId;
    deadline = m->deadline;
    customFields = m->customFields;
}

bool Task::hasFailedPrerequisite() const {
    for(std::size_t i=0; i < dependencies.size();i++){
        const Task* prerequisite = dynamic_cast<const Task*>(dependencies[i]);
        if(prerequisite != nullptr && prerequisite->currentState == FailedState::instance()){
            return true;
        }
    }

    return false;
}

bool Task::isActive() const {
    return currentState == PendingState::instance() ||
           currentState == InProgressState::instance() ||
           currentState == BlockedState::instance();
}

void Task::evaluateDependencies(){
    recomputeReadiness();

    bool prerequisiteFailed = hasFailedPrerequisite();
    if (prerequisiteFailed && isActive()) {
        setState(FailedState::instance());
        failedByPrerequisite = true;
    } else if (!prerequisiteFailed && failedByPrerequisite && currentState == FailedState::instance()) {
        setState(PendingState::instance());
    }
}

bool Task::isCompleted() const {
    return currentState->isSatisfied();
}

std::string Task::getStatusSummary() const {
    return "Task [" + id +"] " + name + " - Status: " +currentState->toString();
}

void Task::collectWork(std::vector<WorkItemView>& out) const {
    bool finished = isCompleted() || currentState== CancelledState::instance();
    if (!isReady() || finished) {
        return;
    }
    WorkItemView view;
    view.id = id;
    view.name =name;
    view.status =currentState->toString();
    view.executor = executorId; 
    view.ready= isReady();
    out.push_back(view);
}

nlohmann::json Task::toJson() const {
    nlohmann::json j;
    j["type"] = "Task";
    j["id"] = id;
    j["name"] = name;
    j["description"] = description;
    j["status"] = currentState->toString();

    nlohmann::json executorJson;
    executorJson["id"] = executorId;
    executorJson["name"] = "";
    executorJson["role"] = "";
    for (std::size_t i = 0; i < participants.size(); ++i) {
        if (participants[i].getId() == executorId) {
            executorJson["name"] = participants[i].getName();
            executorJson["role"] = participants[i].getRole();
            break;
        }
    }
    j["executor"] = executorJson;

    j["deadline"] = deadline.toString("%Y-%m-%d %H:%M");

    nlohmann::json fields = nlohmann::json::object();
    for (std::map<std::string, std::string>::const_iterator it = customFields.begin();
         it != customFields.end(); ++it) {
        fields[it->first] = it->second;
    }
    j["customFields"] = fields;

    nlohmann::json prereqIds = nlohmann::json::array();
    for (std::size_t i = 0; i < dependencies.size(); ++i) {
        prereqIds.push_back(dependencies[i]->getId());
    }
    j["dependsOn"] = prereqIds;
    return j;
}

WorkflowComponent* Task::clone() const {
    Task* clonedTask = nullptr;
    try {
        clonedTask = new Task(*this);
    } catch (...) {
        delete clonedTask;
        throw;
    }
    return clonedTask;
}