#include "Task.h"
#include "Support.h"
#include "TaskState.h"
#include "TaskMemento.h"
#include "WorkflowError.h"
#include <sstream>

int Task::counter = 0;

Task::Task() : currentState(PendingState::instance()) {
    id = generateId();
}

Task::Task(const Task& other)
    : WorkflowComponent(other),
      currentState(PendingState::instance()) {
    id = generateId();
}

Task::~Task() {
    // currentState is a flyweight pointer, do not delete.
}

std::string Task::generateId() {
    std::ostringstream out;
    out << "T-" << ++counter;
    return out.str();
}

void Task::start()    { currentState->start(this); }
void Task::complete() { currentState->complete(this); }
void Task::block()    { currentState->block(this); }
void Task::fail()     { currentState->fail(this); }
void Task::cancel()   { currentState->cancel(this); }
void Task::skip()     { currentState->skip(this); }

TaskState* Task::getState() const {
    return currentState;
}

void Task::setState(TaskState* state) {
    if (state == nullptr) {
        throw WorkflowError("Task::setState: state must not be null");
    }
    currentState = state;
    notifyDependents();
}

TaskMemento* Task::createMemento() const {
    TaskMemento* m = nullptr;
    try {
        m = new TaskMemento(*this);
    } catch (...) {
        delete m;
        throw;
    }
    return m;
}

void Task::restoreMemento(const TaskMemento* m) {
    if (m == nullptr || !m->isValid()) {
        throw WorkflowError("Task::restoreMemento: memento is null or empty");
    }
    name = m->getName();
    description = m->getDescription();
    executor = m->getExecutor();
    deadline = m->getDeadline();
    participants = m->getParticipants();
    customFields = m->getCustomFields();
}

void Task::evaluateDependencies() {
    recomputeReadiness();
}

bool Task::isCompleted() const {
    return currentState->isSatisfied();
}

std::string Task::getStatusSummary() const {
    return "Task [" + id + "] " + name + " - Status: " + currentState->toString();
}

void Task::collectWork(std::vector<WorkItemView>& out) const {
    bool finished = isCompleted() || currentState == CancelledState::instance();
    if (!isReady() || finished) {
        return;
    }
    WorkItemView view;
    view.id = id;
    view.name = name;
    view.status = currentState->toString();
    view.executor = executor.getName();
    view.ready = isReady();
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
    executorJson["id"] = executor.getId();
    executorJson["name"] = executor.getName();
    executorJson["role"] = executor.getRole();
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