/**
 * @file test_command.cpp
 * @brief Command pattern: lifecycle actions, structure edits, task edits, decisions and undo/redo.
 */
#include <gtest/gtest.h>

#include <string>

#include "ActionRegistry.h"
#include "Command.h"
#include "DecisionTask.h"
#include "Project.h"
#include "RoutingStrategy.h"
#include "Support.h"
#include "Task.h"
#include "TaskState.h"
#include "WorkflowError.h"

namespace {

class CommandTest : public ::testing::Test {
protected:
    Project project;
    CommandHistory history;
    ActionRegistry actions;
    FieldValueRoutingStrategy router{"outcome"};

    /** @brief Runs a command the way the Facade does: the caller deletes it if it is rejected. */
    void run(Command* cmd) {
        try {
            history.executeCommand(cmd);
        } catch (...) {
            delete cmd;
            throw;
        }
    }

    Task* addTask(const std::string& name) {
        Task* t = new Task();
        t->setName(name);
        run(new AddWorkItemCommand(&project, t));
        return t;
    }

    DecisionTask* addDecision(const std::string& name) {
        DecisionTask* d = new DecisionTask();
        d->setName(name);
        d->setRouter(&router);
        run(new AddWorkItemCommand(&project, d));
        return d;
    }

    void act(Task* t, const std::string& action) {
        run(actions.create(action, t, nlohmann::json()));
    }

    static std::string state(const Task* t) { return t->getState()->toString(); }
};

}  // namespace

// ---------------------------------------------------------------- TaskStateCommand

TEST_F(CommandTest, StartAndCompleteFollowTheStatePattern) {
    Task* t = addTask("Review");
    act(t, "start");
    EXPECT_EQ("In Progress", state(t));
    act(t, "complete");
    EXPECT_EQ("Complete", state(t));
    EXPECT_TRUE(t->isCompleted());
}

TEST_F(CommandTest, UndoAndRedoRestoreLifecycleState) {
    Task* t = addTask("Review");
    act(t, "start");
    act(t, "complete");

    history.undo();
    EXPECT_EQ("In Progress", state(t));
    history.redo();
    EXPECT_EQ("Complete", state(t));
}

TEST_F(CommandTest, UndoingCompletionMakesDependentNotReadyAgain) {
    Task* a = addTask("A");
    Task* b = addTask("B");
    run(new AddDependencyCommand(b, a));
    act(a, "start");
    act(a, "complete");
    EXPECT_TRUE(b->isReady());

    history.undo();
    EXPECT_FALSE(b->isReady());
}

TEST_F(CommandTest, IllegalTransitionIsRejectedWithoutChanges) {
    Task* t = addTask("Review");
    EXPECT_THROW(act(t, "complete"), InvalidTransitionError);
    EXPECT_EQ("Pending", state(t));

    history.undo();
    EXPECT_TRUE(project.getWorkItems().empty());
}

TEST_F(CommandTest, StartIsRejectedUntilPrerequisitesAreComplete) {
    Task* a = addTask("A");
    Task* b = addTask("B");
    run(new AddDependencyCommand(b, a));

    EXPECT_THROW(act(b, "start"), InvalidTransitionError);
    EXPECT_EQ("Pending", state(b));
}

TEST_F(CommandTest, BlockAndCancelFromInProgress) {
    Task* a = addTask("A");
    Task* b = addTask("B");
    act(a, "start");
    act(a, "block");
    EXPECT_EQ("Blocked", state(a));
    act(b, "start");
    act(b, "cancel");
    EXPECT_EQ("Cancelled", state(b));
}

TEST_F(CommandTest, FailThenRetry) {
    Task* t = addTask("Payment");
    act(t, "start");
    act(t, "fail");
    EXPECT_EQ("Failed", state(t));
    act(t, "start");
    EXPECT_EQ("In Progress", state(t));
}

// ---------------------------------------------------------------- ActionRegistry

TEST_F(CommandTest, RegistryHasBuiltInLifecycleActions) {
    EXPECT_TRUE(actions.has("start"));
    EXPECT_TRUE(actions.has("complete"));
    EXPECT_TRUE(actions.has("block"));
    EXPECT_TRUE(actions.has("fail"));
    EXPECT_TRUE(actions.has("cancel"));
    EXPECT_FALSE(actions.has("approve"));
}

TEST_F(CommandTest, UnknownActionThrowsNotFound) {
    Task* t = addTask("Review");
    EXPECT_THROW(actions.create("teleport", t, nlohmann::json()), NotFoundError);
}

TEST_F(CommandTest, IndustryActionCanBeRegistered) {
    actions.registerAction("approve", [](Task* task, const nlohmann::json&) -> Command* {
        return new TaskStateCommand(task, [](Task& t) { t.complete(); }, "Approve");
    });
    Task* t = addTask("Claim review");
    act(t, "start");
    act(t, "approve");
    EXPECT_EQ("Complete", state(t));
}

// ---------------------------------------------------------------- AddWorkItemCommand

TEST_F(CommandTest, AddWorkItemUndoAndRedo) {
    Task* t = addTask("Review");
    EXPECT_EQ(1u, project.getWorkItems().size());

    history.undo();
    EXPECT_TRUE(project.getWorkItems().empty());
    history.redo();
    ASSERT_EQ(1u, project.getWorkItems().size());
    EXPECT_EQ(t, project.getWorkItems()[0]);
}

TEST_F(CommandTest, UndoneAddIsFreedWhenRedoIsDiscarded) {
    addTask("Temporary");
    history.undo();
    addTask("Replacement");
    EXPECT_FALSE(history.canRedo());
    EXPECT_EQ(1u, project.getWorkItems().size());
}

// ---------------------------------------------------------------- AddDependencyCommand

TEST_F(CommandTest, AddDependencyUndoRemovesTheLink) {
    Task* a = addTask("A");
    Task* b = addTask("B");
    run(new AddDependencyCommand(b, a));
    EXPECT_FALSE(b->isReady());

    history.undo();
    EXPECT_TRUE(b->getDependencies().empty());
    EXPECT_TRUE(b->isReady());
}

TEST_F(CommandTest, CyclicDependencyIsRejectedWithoutChanges) {
    Task* a = addTask("A");
    Task* b = addTask("B");
    run(new AddDependencyCommand(b, a));

    EXPECT_THROW(run(new AddDependencyCommand(a, b)), CyclicDependencyError);
    EXPECT_TRUE(a->getDependencies().empty());
}

// ---------------------------------------------------------------- EditTaskInfoCommand (Memento)

TEST_F(CommandTest, EditTaskUndoAndRedo) {
    Task* t = addTask("Review");
    nlohmann::json payload = {{"name", "Review claim"},
                              {"description", "Check documents"},
                              {"customFields", {{"priority", "high"}}}};

    run(new EditTaskInfoCommand(t, payload));
    EXPECT_EQ("Review claim", t->getName());
    EXPECT_EQ("high", t->getCustomField("priority"));

    history.undo();
    EXPECT_EQ("Review", t->getName());
    EXPECT_EQ("", t->getDescription());

    history.redo();
    EXPECT_EQ("Review claim", t->getName());
    EXPECT_EQ("Check documents", t->getDescription());
}

TEST_F(CommandTest, EditAssignsExecutor) {
    Task* t = addTask("Assess claim");
    nlohmann::json payload = {{"executor", {{"id", "u7"}, {"name", "Thandi"}, {"role", "Assessor"}}}};

    run(new EditTaskInfoCommand(t, payload));
    EXPECT_EQ("u7", t->getExecutorId());
    history.undo();
    EXPECT_EQ("", t->getExecutorId());
}

TEST_F(CommandTest, EditWithUnknownFieldChangesNothing) {
    Task* t = addTask("Review");
    nlohmann::json payload = {{"name", "New"}, {"colour", "blue"}};

    EXPECT_THROW(run(new EditTaskInfoCommand(t, payload)), WorkflowError);
    EXPECT_EQ("Review", t->getName());
}

TEST_F(CommandTest, EditWithMalformedValueIsAWorkflowError) {
    Task* t = addTask("Review");
    nlohmann::json payload = {{"deadline", {{"year", "soon"}}}};
    EXPECT_THROW(run(new EditTaskInfoCommand(t, payload)), WorkflowError);
}

// ---------------------------------------------------------------- DecideCommand

TEST_F(CommandTest, DecisionSkipsOtherBranchesAndCompletesDecision) {
    DecisionTask* d = addDecision("Route claim");
    Task* approve = addTask("Pay out");
    Task* reject = addTask("Send rejection");
    d->addBranchTarget("approve", approve);
    d->addBranchTarget("reject", reject);
    act(d, "start");

    run(new DecideCommand(d, {{"outcome", "approve"}}));
    EXPECT_EQ("approve", d->getSelectedBranch());
    EXPECT_EQ("Complete", state(d));
    EXPECT_EQ("Skipped", state(reject));
    EXPECT_EQ("Pending", state(approve));
    EXPECT_TRUE(approve->isReady());
}

TEST_F(CommandTest, UndoDecisionRestoresEverything) {
    DecisionTask* d = addDecision("Route claim");
    Task* approve = addTask("Pay out");
    Task* reject = addTask("Send rejection");
    d->addBranchTarget("approve", approve);
    d->addBranchTarget("reject", reject);
    act(d, "start");
    run(new DecideCommand(d, {{"outcome", "approve"}}));

    history.undo();
    EXPECT_EQ("", d->getSelectedBranch());
    EXPECT_EQ("In Progress", state(d));
    EXPECT_EQ("Pending", state(reject));
    EXPECT_FALSE(approve->isReady());

    history.redo();
    EXPECT_EQ("Skipped", state(reject));
}

TEST_F(CommandTest, DecisionWithUnknownBranchChangesNothing) {
    DecisionTask* d = addDecision("Route claim");
    d->addBranchTarget("approve", addTask("Pay out"));
    act(d, "start");

    EXPECT_THROW(run(new DecideCommand(d, {{"outcome", "maybe"}})), NotFoundError);
    EXPECT_EQ("In Progress", state(d));
    EXPECT_EQ("", d->getSelectedBranch());
}

TEST_F(CommandTest, DecisionBeforeStartIsRejected) {
    DecisionTask* d = addDecision("Route claim");
    Task* reject = addTask("Send rejection");
    d->addBranchTarget("approve", addTask("Pay out"));
    d->addBranchTarget("reject", reject);

    EXPECT_THROW(run(new DecideCommand(d, {{"outcome", "approve"}})), InvalidTransitionError);
    EXPECT_EQ("Pending", state(reject));
}

TEST_F(CommandTest, SecondDecisionIsRejectedUntilUndone) {
    DecisionTask* d = addDecision("Route claim");
    d->addBranchTarget("approve", addTask("Pay out"));
    d->addBranchTarget("reject", addTask("Send rejection"));
    act(d, "start");
    run(new DecideCommand(d, {{"outcome", "approve"}}));
    act(d, "start");

    EXPECT_THROW(run(new DecideCommand(d, {{"outcome", "reject"}})), InvalidTransitionError);
    EXPECT_EQ("approve", d->getSelectedBranch());
}

// ---------------------------------------------------------------- CommandHistory

TEST_F(CommandTest, UndoAndRedoOnEmptyHistoryThrow) {
    CommandHistory empty;
    EXPECT_FALSE(empty.canUndo());
    EXPECT_FALSE(empty.canRedo());
    EXPECT_THROW(empty.undo(), WorkflowError);
    EXPECT_THROW(empty.redo(), WorkflowError);
}

TEST_F(CommandTest, NewActionClearsRedo) {
    Task* t = addTask("Review");
    act(t, "start");
    history.undo();
    EXPECT_TRUE(history.canRedo());

    act(t, "start");
    EXPECT_FALSE(history.canRedo());
}

TEST_F(CommandTest, DescribeNamesTheAction) {
    Task* t = addTask("Review");
    Command* cmd = actions.create("start", t, nlohmann::json());
    EXPECT_NE(std::string::npos, cmd->describe().find(t->getId()));
    delete cmd;
}
