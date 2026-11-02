#include "RemoveWorkItemCommand.h"
#include "DecisionTask.h"
#include "Project.h"
#include "Task.h"
#include "WorkflowComponent.h"
#include "WorkflowError.h"

#include <algorithm>
#include <map>

RemoveWorkItemCommand::RemoveWorkItemCommand(Project* parent, WorkflowComponent* child)
    : parent(parent), child(child), removed(false) {}

RemoveWorkItemCommand::~RemoveWorkItemCommand(){
    if (removed) delete child;   
}

void RemoveWorkItemCommand::execute() {
    if (child == nullptr || parent == nullptr) {
        throw WorkflowError("RemoveWorkItemCommand: nothing to remove");
    }

    prerequisites =child->getDependencies();
    dependents.clear();
    branchLinks.clear();
    Task* childTask = dynamic_cast<Task*>(child);
    const std::vector<WorkflowComponent*>& siblings = parent->getWorkItems();
    for (std::size_t i = 0; i < siblings.size(); ++i) {
        if (siblings[i] == child) continue;

        const std::vector<WorkflowComponent*>& waitsFor = siblings[i]->getDependencies();
        if (std::find(waitsFor.begin(), waitsFor.end(), child) != waitsFor.end()){
            dependents.push_back(siblings[i]);
        }

        DecisionTask* decision = dynamic_cast<DecisionTask*>(siblings[i]);
        if (decision != nullptr && childTask != nullptr) {
            const std::map<std::string, std::vector<Task*> >& branches = decision->getBranches();
            for (std::map<std::string, std::vector<Task*> >::const_iterator b = branches.begin(); b != branches.end(); ++b) {
                if (std::find(b->second.begin(), b->second.end(), childTask) != b->second.end()) {
                    BranchLink link;
                    link.decision = decision;
                    link.label = b->first;
                    branchLinks.push_back(link);
                }
            }
        }
    }

    for (std::size_t i = 0; i< branchLinks.size(); ++i){
        branchLinks[i].decision->removeBranchTarget(childTask);
    }
    parent->removeWorkItem(child); 
    removed = true;
}

void RemoveWorkItemCommand::undo(){
    parent->addWorkItem(child);
    removed = false;

    Task* childTask = dynamic_cast<Task*>(child);
    for (std::size_t i = 0; i < prerequisites.size(); ++i) {
        child->addDependency(prerequisites[i]);
    }
    for (std::size_t i = 0; i < dependents.size(); ++i) {
        dependents[i]->addDependency(child);
    }
    for (std::size_t i = 0; i < branchLinks.size(); ++i){
        branchLinks[i].decision->addBranchTarget(branchLinks[i].label, childTask);
    }
}

std::string RemoveWorkItemCommand::describe() const{
    return "Remove work item " + child->getName();
}
