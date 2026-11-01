#ifndef TIVIDY_REMOVE_WORK_ITEM_COMMAND_H
#define TIVIDY_REMOVE_WORK_ITEM_COMMAND_H

#include "Command.h"

#include <string>
#include <vector>

class DecisionTask;
class Project;
class WorkflowComponent;

/**
 * @file RemoveWorkItemCommand.h
 * @brief Undoable removal of a work item from a project.
 *
 * While the item is removed this command owns it, so commands earlier in the history that still point at it
 * stay valid. Undo puts the item back and restores everything removal cut: its prerequisites, the tasks that
 * were waiting for it, and its place in any decision's branches.
 */
class RemoveWorkItemCommand : public Command {
private:
    struct BranchLink {
        DecisionTask* decision;
        std::string label;
    };

    Project* parent;                                  ///< Project the item is removed from.
    WorkflowComponent* child;                         ///< The item being removed.
    bool removed;                                     ///< True while the item is out of the project (owned here).
    std::vector<WorkflowComponent*> prerequisites;    ///< Items the child waited for.
    std::vector<WorkflowComponent*> dependents;       ///< Items that waited for the child.
    std::vector<BranchLink> branchLinks;              ///< Decision branches that listed the child.

public:
    RemoveWorkItemCommand(Project* parent, WorkflowComponent* child);
    virtual ~RemoveWorkItemCommand();

    void execute() override;
    void undo() override;
    std::string describe() const override;
};

#endif // TIVIDY_REMOVE_WORK_ITEM_COMMAND_H
