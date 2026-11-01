#include "DecisionTask.h"
#include "RoutingStrategy.h"
#include "WorkflowComponent.h"
#include "WorkflowError.h"
#include <algorithm>
#include <stdexcept>

DecisionTask::DecisionTask() : router(nullptr) {}

DecisionTask::DecisionTask(const DecisionTask& other) : Task(other), router(other.router), selectedBranch(other.selectedBranch) {
    // Deep copy branches map and clone target tasks if owned or copy raw pointers based on architecture
    for (std::map<std::string, std::vector<Task*> >::const_iterator it = other.branches.begin();
         it != other.branches.end(); ++it) {
        std::vector<Task*> clonedTargets;
        for (std::size_t i = 0; i < it->second.size(); ++i) {
            if (it->second[i]) {
                WorkflowComponent* clonedComp = it->second[i]->clone();
                clonedTargets.push_back(dynamic_cast<Task*>(clonedComp));
            }
        }
        branches[it->first] = clonedTargets;
    }
}

DecisionTask::~DecisionTask() {
    router = nullptr;
    branches.clear();
}

void DecisionTask::setRouter(RoutingStrategy* r) {
    router = r;
}

void DecisionTask::addBranchTarget(const std::string& label, Task* target) {
    if (target == nullptr) {
        throw WorkflowError("DecisionTask::addBranchTarget: target must not be null");
    }
    if (label.empty()) {
        throw WorkflowError("DecisionTask::addBranchTarget: branch label must not be empty");
    }

    target->addDependency(this);

    std::vector<Task*>& targets = branches[label];

    if (std::find(targets.begin(), targets.end(), target) == targets.end()) {
        targets.push_back(target);
    }
}

std::vector<Task*> DecisionTask::getBranchTargets() const {
    std::vector<Task*> all;
    for (std::map<std::string, std::vector<Task*> >::const_iterator it = branches.begin();
         it != branches.end(); ++it) {
        all.insert(all.end(), it->second.begin(), it->second.end());
    }
    return all;
}

const std::map<std::string, std::vector<Task*> >& DecisionTask::getBranches() const {
    return branches;
}

std::string DecisionTask::decide(const nlohmann::json& context) {
    if (router == nullptr) {
        throw NotFoundError("DecisionTask::decide: no router set for " + getId());
    }
    std::string label = router->route(*this, context);   // STRATEGY: the router chooses
    if (branches.find(label) == branches.end()) {
        throw NotFoundError("DecisionTask::decide: unknown branch \"" + label + "\"");
    }
    selectedBranch = label;
    return selectedBranch;
}

void DecisionTask::clearDecision() {
    selectedBranch.clear();
}

const std::string& DecisionTask::getSelectedBranch() const {
    return selectedBranch;
}

nlohmann::json DecisionTask::toJson() const {
    nlohmann::json j = Task::toJson();    // reuse everything a Task already exports
    j["type"] = "Decision";
    j["selectedBranch"] = selectedBranch;

    nlohmann::json branchesJson = nlohmann::json::object();
    for (std::map<std::string, std::vector<Task*> >::const_iterator it = branches.begin();
         it != branches.end(); ++it) {
        nlohmann::json ids = nlohmann::json::array();
        for (std::size_t i = 0; i < it->second.size(); ++i) {
            if (it->second[i]) {
                ids.push_back(it->second[i]->getId());
            }
        }
        branchesJson[it->first] = ids;
    }
    j["branches"] = branchesJson;
    return j;
}

WorkflowComponent* DecisionTask::clone() const {
    DecisionTask* clonedDecision = nullptr;
    try {
        clonedDecision = new DecisionTask(*this);
    } catch (...) {
        delete clonedDecision;
        throw;
    }
    return clonedDecision;
}