#include "../include/WorkflowComponent.h"
#include "../include/WorkflowError.h"
#include <algorithm>
#include <set>
#include <stdexcept>

namespace {

bool canReach(const WorkflowComponent* from,
              const WorkflowComponent* target,
              std::set<const WorkflowComponent*>& seen) {
    if (from == target) {
        return true;
    }
    if (!seen.insert(from).second) {
        return false;
    }
    const std::vector<WorkflowComponent*>& prereqs = from->getDependencies();
    for (std::size_t i = 0; i < prereqs.size(); ++i) {
        if (canReach(prereqs[i], target, seen)) {
            return true;
        }
    }
    return false;
}

}

WorkflowComponent::WorkflowComponent() : ready_(true) {
}

WorkflowComponent::WorkflowComponent(const WorkflowComponent& other)
    : id_(other.id_),
      name_(other.name_),
      description_(other.description_),
      executor_(other.executor_),
      deadline_(other.deadline_),
      customFields_(other.customFields_),
      participants_(other.participants_),
      ready_(true) {
}

WorkflowComponent::~WorkflowComponent() {
}

std::string WorkflowComponent::getId() const { return id_; }
std::string WorkflowComponent::getName() const { return name_; }
void WorkflowComponent::setName(const std::string& name) { name_ = name; }
std::string WorkflowComponent::getDescription() const { return description_; }
void WorkflowComponent::setDescription(const std::string& desc) { description_ = desc; }

void WorkflowComponent::setExecutor(const Participant& p) { executor_ = p; }
std::string WorkflowComponent::getExecutorId() const { return executor_.getId(); }

std::string WorkflowComponent::addParticipant(Participant p) {
    for (std::size_t i = 0; i < participants_.size(); ++i) {
        if (participants_[i].getId() == p.getId()) {
            return p.getId();
        }
    }
    participants_.push_back(p);
    return p.getId();
}

void WorkflowComponent::setDeadline(const DateTime& d) { deadline_ = d; }

void WorkflowComponent::addComment(Comment comment) { comments_.push_back(comment); }

void WorkflowComponent::setCustomField(const std::string& key, const std::string& value) {
    customFields_[key] = value;
}

std::string WorkflowComponent::getCustomField(const std::string& key) const {
    std::map<std::string, std::string>::const_iterator it = customFields_.find(key);
    if (it == customFields_.end()) {
        return "";
    }
    return it->second;
}

bool WorkflowComponent::addDependency(WorkflowComponent* prerequisite) {
    if (prerequisite == nullptr) {
        throw std::logic_error("WorkflowComponent::addDependency: prerequisite must not be null");
    }
    if (prerequisite == this) {
        throw CyclicDependencyError("A component cannot depend on itself: " + id_);
    }

    if (std::find(dependencies_.begin(), dependencies_.end(), prerequisite) != dependencies_.end()) {
        return false;
    }

    std::set<const WorkflowComponent*> seen;
    if (canReach(prerequisite, this, seen)) {
        throw CyclicDependencyError("Dependency would create a cycle: " + id_ +
                                    " -> " + prerequisite->getId());
    }

    dependencies_.push_back(prerequisite);
    prerequisite->addDependent(this);
    evaluateDependencies();
    return true;
}

void WorkflowComponent::removeDependency(WorkflowComponent* prerequisite) {
    if (prerequisite == nullptr) {
        return;
    }
    std::vector<WorkflowComponent*>::iterator it =
        std::find(dependencies_.begin(), dependencies_.end(), prerequisite);
    if (it == dependencies_.end()) {
        return;
    }
    dependencies_.erase(it);
    prerequisite->removeDependent(this);
    evaluateDependencies();
}

void WorkflowComponent::addDependent(WorkflowComponent* component) {
    if (component == nullptr) {
        throw std::logic_error("WorkflowComponent::addDependent: component must not be null");
    }
    if (std::find(dependents_.begin(), dependents_.end(), component) == dependents_.end()) {
        dependents_.push_back(component);
    }
}

void WorkflowComponent::removeDependent(WorkflowComponent* component) {
    dependents_.erase(std::remove(dependents_.begin(), dependents_.end(), component),
                      dependents_.end());
}

const std::vector<WorkflowComponent*>& WorkflowComponent::getDependencies() const {
    return dependencies_;
}

void WorkflowComponent::notifyDependents() {
    for (std::size_t i = 0; i < dependents_.size(); ++i) {
        dependents_[i]->update(this);
    }
}

void WorkflowComponent::update(WorkflowComponent*) {
    evaluateDependencies();
}

bool WorkflowComponent::isReady() const {
    return ready_;
}

void WorkflowComponent::recomputeReadiness() {
    bool allDone = true;
    for (std::size_t i = 0; i < dependencies_.size(); ++i) {
        if (!dependencies_[i]->isCompleted()) {
            allDone = false;
            break;
        }
    }
    ready_ = allDone;
}

void WorkflowComponent::unlinkAll() {
    std::vector<WorkflowComponent*> myPrereqs = dependencies_;
    for (std::size_t i = 0; i < myPrereqs.size(); ++i) {
        removeDependency(myPrereqs[i]);
    }

    std::vector<WorkflowComponent*> myDependents = dependents_;
    for (std::size_t i = 0; i < myDependents.size(); ++i) {
        myDependents[i]->removeDependency(this);
    }
}