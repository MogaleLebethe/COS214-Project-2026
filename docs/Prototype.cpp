/**
 * @file Prototype.cpp
 * @brief Prototype pattern: clone() for every workflow component (spec section 4).
 *
 * Rules shared by every clone:
 *  - implemented with a protected copy constructor;
 *  - the copy gets a fresh identifier;
 *  - by-value attributes are copied (name, description, executor, deadline,
 *    custom fields, participants);
 *  - runtime data is reset: state = Pending, comments cleared, no decision;
 *  - leaves are copied with *no* dependency edges, because those would point at
 *    the original's peers. Project::clone() remaps edges whose both ends lie
 *    inside the cloned project, so a cloned instance is fully independent.
 *
 * Ownership: every clone() TRANSFERS ownership of the result to the caller.
 */
#include <utility>

#include "core/DecisionTask.h"
#include "core/Project.h"
#include "core/Task.h"
#include "state/TaskState.h"

namespace {

typedef std::vector<std::pair<const WorkflowComponent*, WorkflowComponent*> > ClonePairs;

/** @brief Walks the original and the copy in parallel (same shape) recording original -> copy. */
void pairTrees(const WorkflowComponent* original, WorkflowComponent* copy,
               WorkflowComponent::CloneMap& map, ClonePairs& order) {
    map[original] = copy;
    order.push_back(std::make_pair(original, copy));
    std::vector<WorkflowComponent*> from = original->getChildren();
    std::vector<WorkflowComponent*> to = copy->getChildren();
    for (std::size_t i = 0; i < from.size() && i < to.size(); ++i) pairTrees(from[i], to[i], map, order);
}

}  // namespace

// ---------------------------------------------------------------- WorkflowComponent

WorkflowComponent::WorkflowComponent(const WorkflowComponent& other)
    : id(),
      name(other.name),
      description(other.description),
      executorId(other.executorId),
      deadline(other.deadline),
      customFields(other.customFields),
      comments(),
      participants(other.participants),
      dependencies(),
      dependents(),
      parent(0),
      ready(true),
      notificationCount(0) {}

void WorkflowComponent::remapLinksFrom(const WorkflowComponent& original, const CloneMap& map) {
    for (std::size_t i = 0; i < original.dependencies.size(); ++i) {
        CloneMap::const_iterator it = map.find(original.dependencies[i]);
        if (it != map.end()) addDependency(it->second);  // edges leaving the clone are dropped
    }
}

// ---------------------------------------------------------------- Task

Task::Task(const Task& other) : WorkflowComponent(other), currentState(PendingState::instance()) {
    setId(generateId());
}

Task* Task::clone() const { return new Task(*this); }

// ---------------------------------------------------------------- DecisionTask

DecisionTask::DecisionTask(const DecisionTask& other)
    : Task(other), router(other.router), branches(), selectedBranch() {}

DecisionTask* DecisionTask::clone() const { return new DecisionTask(*this); }

void DecisionTask::remapLinksFrom(const WorkflowComponent& original, const CloneMap& map) {
    WorkflowComponent::remapLinksFrom(original, map);
    const DecisionTask& source = static_cast<const DecisionTask&>(original);  // same dynamic type
    for (BranchMap::const_iterator it = source.branches.begin(); it != source.branches.end(); ++it) {
        for (std::size_t i = 0; i < it->second.size(); ++i) {
            CloneMap::const_iterator found = map.find(it->second[i]);
            if (found == map.end()) continue;
            Task* target = dynamic_cast<Task*>(found->second);
            if (target != 0) addBranchTarget(it->first, target);
        }
    }
}

// ---------------------------------------------------------------- Project

Project::Project(const Project& other) : WorkflowComponent(other), workItems() {
    setId(generateId());
}

Project* Project::clone() const {
    Project* copy = new Project(*this);  // the single pending pointer (rule O9)
    try {
        for (std::size_t i = 0; i < workItems.size(); ++i) {
            WorkflowComponent* child = workItems[i]->clone();
            try {
                copy->addWorkItem(child);  // ownership moves to the copy immediately
            } catch (...) {
                delete child;
                throw;
            }
        }
        CloneMap map;
        ClonePairs order;
        pairTrees(this, copy, map, order);
        for (std::size_t i = 1; i < order.size(); ++i) {  // index 0 is the root itself
            order[i].second->remapLinksFrom(*order[i].first, map);
        }
    } catch (...) {
        delete copy;  // deletes the children already added
        throw;
    }
    return copy;
}
