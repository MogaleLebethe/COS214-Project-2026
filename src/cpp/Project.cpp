#include "Project.h"
#include "Support.h"
#include "DecisionTask.h"
#include "WorkflowError.h"
#include <algorithm>
#include <map>
#include <sstream>

int Project::counter = 0;

Project::Project() {
    id = generateId();
}

Project::Project(const Project& other) : WorkflowComponent(other) {
    id = generateId();   // The copy is a new project, so it gets a new id
}

Project::~Project() {
    for (std::size_t i = 0; i < workItems.size(); ++i) {
        delete workItems[i];
    }
    workItems.clear();
}

std::string Project::generateId() {
    std::ostringstream out;
    out << "P-" << ++counter;
    return out.str();
}

bool Project::contains(const WorkflowComponent* component) const {
    for (std::size_t i = 0; i < workItems.size(); ++i) {
        if (workItems[i] == component) {
            return true;
        }
        const Project* inner = dynamic_cast<const Project*>(workItems[i]);
        if (inner != nullptr && inner->contains(component)) {
            return true;
        }
    }
    return false;
}

void Project::addWorkItem(WorkflowComponent* item) {
    if (item == nullptr) {
        throw WorkflowError("Project::addWorkItem: item must not be null");
    }
    if (item == this) {
        throw WorkflowError("Project::addWorkItem: a project cannot contain itself");
    }
    if (contains(item)) {
        throw WorkflowError("Project::addWorkItem: item is already part of this project");
    }
    
    const Project* itemAsProject = dynamic_cast<const Project*>(item);
    if (itemAsProject != nullptr && itemAsProject->contains(this)) {
        throw WorkflowError("Project::addWorkItem: that project already contains this project");
    }

    workItems.push_back(item);   
}

WorkflowComponent* Project::removeWorkItem(WorkflowComponent* item) {
    std::vector<WorkflowComponent*>::iterator it =
        std::find(workItems.begin(), workItems.end(), item);
    if (it == workItems.end()) {
        throw NotFoundError("Project::removeWorkItem: item is not a direct child of this project");
    }
    
    item->unlinkAll();
    workItems.erase(it);   
    return item;            
}

const std::vector<WorkflowComponent*>& Project::getWorkItems() const {
    return workItems;
}

WorkflowComponent* Project::findById(const std::string& targetId) const {
    for (std::size_t i = 0; i < workItems.size(); ++i) {
        if (workItems[i]->getId() == targetId) {
            return workItems[i];
        }
        const Project* inner = dynamic_cast<const Project*>(workItems[i]);
        if (inner != nullptr) {
            WorkflowComponent* found = inner->findById(targetId);
            if (found != nullptr) {
                return found;
            }
        }
    }
    return nullptr;
}

void Project::unlinkAll() {
    WorkflowComponent::unlinkAll();                
    for (std::size_t i = 0; i < workItems.size(); ++i) {
        workItems[i]->unlinkAll();             
    }
}

void Project::evaluateDependencies() {
    recomputeReadiness();
}

bool Project::isCompleted() const {
    for (std::size_t i = 0; i < workItems.size(); ++i) {
        if (!workItems[i]->isCompleted()) {
            return false;
        }
    }
    return true;  
}

std::string Project::getStatusSummary() const {
    std::size_t done = 0;
    for (std::size_t i = 0; i < workItems.size(); ++i) {
        if (workItems[i]->isCompleted()) {
            ++done;
        }
    }

    std::ostringstream out;
    out << "Project [" << id << "] " << name << " - "
        << done << "/" << workItems.size() << " complete";

    for (std::size_t i = 0; i < workItems.size(); ++i) {
        std::string childText = workItems[i]->getStatusSummary();
        std::string indented;
        for (std::size_t k = 0; k < childText.size(); ++k) {
            indented += childText[k];
            if (childText[k] == '\n') {
                indented += "  ";
            }
        }
        out << "\n  " << indented;
    }
    return out.str();
}

void Project::collectWork(std::vector<WorkItemView>& out) const {
    for (std::size_t i = 0; i < workItems.size(); ++i) {
        workItems[i]->collectWork(out);
    }
}

nlohmann::json Project::toJson() const {
    nlohmann::json j;
    j["type"] = "Project";
    j["id"] = id;
    j["name"] = name;
    j["description"] = description;
    j["completed"] = isCompleted();

    nlohmann::json fields = nlohmann::json::object();
    for (std::map<std::string, std::string>::const_iterator it = customFields.begin();
         it != customFields.end(); ++it) {
        fields[it->first] = it->second;
    }
    j["customFields"] = fields;

    nlohmann::json items = nlohmann::json::array();
    for (std::size_t i = 0; i < workItems.size(); ++i) {
        items.push_back(workItems[i]->toJson());
    }
    j["workItems"] = items;
    return j;
}

WorkflowComponent* Project::clone() const {
    Project* copy = nullptr;
    try {
        copy = new Project(*this);
        std::map<const WorkflowComponent*, WorkflowComponent*> copyOf;
        for (std::size_t i = 0; i < workItems.size(); ++i) {
            WorkflowComponent* childCopy = workItems[i]->clone();
            try {
                copy->addWorkItem(childCopy);   
            } catch (...) {
                delete childCopy;             
                throw;
            }
            copyOf[workItems[i]] = childCopy;
        }

        for (std::size_t i = 0; i < workItems.size(); ++i) {
            const WorkflowComponent* original = workItems[i];
            WorkflowComponent* duplicate = copyOf[original];

            const std::vector<WorkflowComponent*>& prereqs = original->getDependencies();
            for (std::size_t j = 0; j < prereqs.size(); ++j) {
                std::map<const WorkflowComponent*, WorkflowComponent*>::const_iterator found =
                    copyOf.find(prereqs[j]);
                if (found != copyOf.end()) {
                    duplicate->addDependency(found->second);
                }
            }

            const DecisionTask* originalDecision = dynamic_cast<const DecisionTask*>(original);
            DecisionTask* duplicateDecision = dynamic_cast<DecisionTask*>(duplicate);
            if (originalDecision != nullptr && duplicateDecision != nullptr) {
                const std::map<std::string, std::vector<Task*> >& branches =
                    originalDecision->getBranches();
                for (std::map<std::string, std::vector<Task*> >::const_iterator b = branches.begin();
                     b != branches.end(); ++b) {
                    for (std::size_t t = 0; t < b->second.size(); ++t) {
                        std::map<const WorkflowComponent*, WorkflowComponent*>::const_iterator found =
                            copyOf.find(b->second[t]);
                        Task* targetCopy =
                            (found == copyOf.end()) ? nullptr : dynamic_cast<Task*>(found->second);
                        if (targetCopy != nullptr) {
                            duplicateDecision->addBranchTarget(b->first, targetCopy);
                        }
                    }
                }
            }
        }
    } catch (...) {
        delete copy;   
        throw;
    }
    return copy;
}