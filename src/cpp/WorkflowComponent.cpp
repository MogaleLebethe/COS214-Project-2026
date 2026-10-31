#include "WorkflowComponent.h"
#include "WorkflowError.h"
#include "Support.h"
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

WorkflowComponent::WorkflowComponent(const WorkflowComponent& other)
    : id(other.id),
      name(other.name),
      description(other.description),
      executorId(other.executorId),
      deadline(other.deadline),
      customFields(other.customFields),
      comments(other.comments),
      participants(other.participants),
      ready(true) {
}

WorkflowComponent::~WorkflowComponent() {
}

std::string WorkflowComponent::getId() const { return id; }
std::string WorkflowComponent::getName() const { return name; }
void WorkflowComponent::setName(const std::string& componentName) { name = componentName; }
std::string WorkflowComponent::getDescription() const { return description; }
void WorkflowComponent::setDescription(const std::string& desc) { description = desc; }

void WorkflowComponent::setExecutor(const Participant& p) { 
    executorId = p.getId(); 
}

std::string WorkflowComponent::getExecutorId() const { return executorId; }

std::string WorkflowComponent::addParticipant(Participant p) {
    for (std::size_t i = 0; i < participants.size(); ++i) {
        if (participants[i].getId() == p.getId()) {
            return p.getId();
        }
    }
    participants.push_back(p);
    return p.getId();
}

void WorkflowComponent::setDeadline(const DateTime& d) { deadline = d; }

void WorkflowComponent::addComment(Comment comment) { comments.push_back(comment); }

void WorkflowComponent::setCustomField(const std::string& key, const std::string& value) {
    customFields[key] = value;
}

std::string WorkflowComponent::getCustomField(const std::string& key) const {
    std::map<std::string, std::string>::const_iterator it = customFields.find(key);
    if (it == customFields.end()) {
        return "";
    }
    return it->second;
}

bool WorkflowComponent::addDependency(WorkflowComponent* prerequisite) {
    if (prerequisite == nullptr) {
        throw std::logic_error("WorkflowComponent::addDependency: prerequisite must not be null");
    }
    if (prerequisite == this) {
        throw CyclicDependencyError("A component cannot depend on itself: " + id);
    }

    if (std::find(dependencies.begin(), dependencies.end(), prerequisite) != dependencies.end()) {
        return false;
    }

    std::set<const WorkflowComponent*> seen;
    if (canReach(prerequisite, this, seen)) {
        throw CyclicDependencyError("Dependency would create a cycle: " + id +
                                    " -> " + prerequisite->getId());
    }

    dependencies.push_back(prerequisite);
    prerequisite->addDependent(this);
    evaluateDependencies();
    return true;
}

void WorkflowComponent::removeDependency(WorkflowComponent* prerequisite) {
    if (prerequisite == nullptr) {
        return;
    }
    std::vector<WorkflowComponent*>::iterator it =
        std::find(dependencies.begin(), dependencies.end(), prerequisite);
    if (it == dependencies.end()) {
        return;
    }
    dependencies.erase(it);
    prerequisite->removeDependent(this);
    evaluateDependencies();
}

void WorkflowComponent::addDependent(WorkflowComponent* component) {
    if (component == nullptr) {
        throw std::logic_error("WorkflowComponent::addDependent: component must not be null");
    }
    if (std::find(dependents.begin(), dependents.end(), component) == dependents.end()) {
        dependents.push_back(component);
    }
}

void WorkflowComponent::removeDependent(WorkflowComponent* component) {
    dependents.erase(std::remove(dependents.begin(), dependents.end(), component),
                      dependents.end());
}

const std::vector<WorkflowComponent*>& WorkflowComponent::getDependencies() const {
    return dependencies;
}

void WorkflowComponent::notifyDependents() {
    for (std::size_t i = 0; i < dependents.size(); ++i) {
        dependents[i]->update(this);
    }
}

void WorkflowComponent::update(WorkflowComponent*) {
    evaluateDependencies();
}

bool WorkflowComponent::isReady() const {
    return ready;
}

void WorkflowComponent::recomputeReadiness() {
    bool allDone = true;
    for (std::size_t i = 0; i < dependencies.size(); ++i) {
        if (!dependencies[i]->isCompleted()) {
            allDone = false;
            break;
        }
    }
    ready = allDone;
}

void WorkflowComponent::unlinkAll() {
    std::vector<WorkflowComponent*> myPrereqs = dependencies;
    for (std::size_t i = 0; i < myPrereqs.size(); ++i) {
        removeDependency(myPrereqs[i]);
    }

    std::vector<WorkflowComponent*> myDependents = dependents;
    for (std::size_t i = 0; i < myDependents.size(); ++i) {
        myDependents[i]->removeDependency(this);
    }
}