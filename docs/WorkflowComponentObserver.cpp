/**
 * @file WorkflowComponentObserver.cpp
 * @brief Observer pattern: dependency evaluation and cascade (spec section 8).
 *
 * A component is the *subject* for its dependents and the *observer* of its
 * dependencies. When a task changes state, Task::setState() calls
 * notifyDependents(); every dependent re-computes its cached readiness in
 * update(). Observers only recompute readiness; they never start, block or
 * complete a task, so every state change still originates from a Command and
 * undo/redo stays exact.
 *
 * Composite interplay:
 *  - a Project's completion depends on its children, so a child's notification
 *    is forwarded to the parent's dependents;
 *  - a child is only ready while every enclosing project is ready.
 */
#include <algorithm>
#include <stdexcept>

#include "common/Errors.h"
#include "core/WorkflowComponent.h"

bool WorkflowComponent::addDependency(WorkflowComponent* prerequisite) {
    if (prerequisite == 0) throw std::logic_error("addDependency: prerequisite is null");
    if (prerequisite == this) {
        throw CyclicDependencyError("'" + name + "' cannot depend on itself");
    }
    if (std::find(dependencies.begin(), dependencies.end(), prerequisite) != dependencies.end()) {
        return false;
    }
    // After the link, this component and everything it contains waits for the
    // prerequisite. A cycle exists if the prerequisite already waits for any of them.
    std::set<const WorkflowComponent*> waiting;
    collectSubtree(waiting);
    if (waiting.count(prerequisite) || prerequisite->waitsForAny(waiting)) {
        throw CyclicDependencyError("Linking '" + name + "' to '" + prerequisite->getName() +
                                    "' would create a dependency cycle");
    }
    dependencies.push_back(prerequisite);
    prerequisite->addDependent(this);
    evaluateDependencies();
    return true;
}

void WorkflowComponent::removeDependency(WorkflowComponent* prerequisite) {
    if (std::find(dependencies.begin(), dependencies.end(), prerequisite) == dependencies.end()) return;
    eraseDependency(prerequisite);
    prerequisite->removeDependent(this);
    evaluateDependencies();
}

void WorkflowComponent::notifyDependents() {
    // Copy first: an observer must be free to change the list it is notified from.
    std::vector<WorkflowComponent*> snapshot(dependents);
    for (std::size_t i = 0; i < snapshot.size(); ++i) snapshot[i]->update(this);
    // The parent's completion may have changed with ours.
    if (parent != 0) parent->notifyDependents();
}

void WorkflowComponent::update(WorkflowComponent* /*changed*/) {
    ++notificationCount;
    evaluateDependencies();
}

bool WorkflowComponent::isReady() const {
    return ready && (parent == 0 || parent->isReady());
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

bool WorkflowComponent::unlinkAll() {
    bool changed = !dependencies.empty() || !dependents.empty();
    for (std::size_t i = 0; i < dependencies.size(); ++i) dependencies[i]->removeDependent(this);
    dependencies.clear();
    std::vector<WorkflowComponent*> former(dependents);
    dependents.clear();
    for (std::size_t i = 0; i < former.size(); ++i) {
        former[i]->eraseDependency(this);
        former[i]->evaluateDependencies();
    }
    evaluateDependencies();
    return changed;
}

bool WorkflowComponent::waitsForAny(const std::set<const WorkflowComponent*>& targets) const {
    // Depth-first search over "waits for" edges: own dependencies, the
    // dependencies of every enclosing project, and (for projects) the children.
    std::vector<const WorkflowComponent*> stack(1, this);
    std::set<const WorkflowComponent*> visited;
    while (!stack.empty()) {
        const WorkflowComponent* node = stack.back();
        stack.pop_back();
        if (!visited.insert(node).second) continue;
        std::vector<const WorkflowComponent*> next;
        for (const WorkflowComponent* scope = node; scope != 0; scope = scope->parent) {
            next.insert(next.end(), scope->dependencies.begin(), scope->dependencies.end());
        }
        std::vector<WorkflowComponent*> children = node->getChildren();
        next.insert(next.end(), children.begin(), children.end());
        for (std::size_t i = 0; i < next.size(); ++i) {
            if (targets.count(next[i])) return true;
            stack.push_back(next[i]);
        }
    }
    return false;
}

void WorkflowComponent::collectSubtree(std::set<const WorkflowComponent*>& out) const {
    out.insert(this);
    std::vector<WorkflowComponent*> children = getChildren();
    for (std::size_t i = 0; i < children.size(); ++i) children[i]->collectSubtree(out);
}

void WorkflowComponent::addDependent(WorkflowComponent* dependent) {
    dependents.push_back(dependent);
}

void WorkflowComponent::removeDependent(WorkflowComponent* dependent) {
    dependents.erase(std::remove(dependents.begin(), dependents.end(), dependent), dependents.end());
}

void WorkflowComponent::eraseDependency(WorkflowComponent* prerequisite) {
    dependencies.erase(std::remove(dependencies.begin(), dependencies.end(), prerequisite),
                       dependencies.end());
}
