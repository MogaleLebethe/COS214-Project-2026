#include "DecisionTask.h"
#include "RoutingStrategy.h"
#include "WorkflowComponent.h"
#include "WorkflowError.h"
#include <algorithm>
#include <stdexcept>

DecisionTask::DecisionTask() : router(nullptr) {}

DecisionTask::DecisionTask(const DecisionTask& other)
    : Task(other), router(other.router) {
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

void DecisionTask::removeBranchTarget(Task* target) {
    std::map<std::string, std::vector<Task*> >::iterator it = branches.begin();
    while (it != branches.end()) {
        std::vector<Task*>& targets = it->second;
        targets.erase(std::remove(targets.begin(), targets.end(), target), targets.end());
        if (targets.empty()) {
            it = branches.erase(it);
        } else {
            ++it;
        }
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
    std::string label = router->route(*this, context);
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
    nlohmann::json j = Task::toJson();   
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
    return new DecisionTask(*this);
}
