#include "DecideCommand.h"
#include "DecisionTask.h"
#include "Task.h"
#include "TaskState.h"
#include <vector>

DecideCommand::DecideCommand(DecisionTask* task, const nlohmann::json& context)
    : task(task), context(context) {}

void DecideCommand::execute() {
    previousSelection = task->getSelectedBranch();
    previousStates.clear();
    previousStates[task] = task->getState();

    std::vector<Task*> targets = task->getBranchTargets();
    for (std::size_t i = 0; i < targets.size(); ++i)
        previousStates[targets[i]] = targets[i]->getState();

    try {
        std::string label = task->decide(context);
        const std::map<std::string, std::vector<Task*> >& branches = task->getBranches();
        const std::vector<Task*>& selected = branches.at(label);

        for (std::map<std::string, std::vector<Task*> >::const_iterator b = branches.begin(); b != branches.end(); ++b) {
            if (b->first == label) continue;
            for (std::size_t i = 0; i < b->second.size(); ++i) {
                Task* t = b->second[i];
                bool alsoSelected = false;
                for (std::size_t j = 0; j < selected.size() && !alsoSelected; ++j)
                    alsoSelected = (selected[j] == t);
                if (!alsoSelected && !t->isCompleted()) t->skip();
            }
        }
        task->complete();
        chosen = label;
    } catch (...) {
        restore();
        throw;
    }
}

void DecideCommand::undo() {
    restore();
}

std::string DecideCommand::describe() const {
    std::string text = "Decide " + task->getId();
    return chosen.empty() ? text : text + " -> " + chosen;
}

void DecideCommand::restore() {
    for (std::map<Task*, TaskState*>::iterator it = previousStates.begin(); it != previousStates.end(); ++it)
        it->first->setState(it->second);

    if (previousSelection.empty()) task->clearDecision();
    else task->setSelectedBranch(previousSelection);
}
