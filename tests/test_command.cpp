/**
 * @file test_command.cpp
 * @brief Command pattern (doctest): lifecycle actions, structure edits, task edits, decisions and undo/redo.
 */
#include "doctest/doctest.h"

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

struct CommandTest {
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

TEST_SUITE_BEGIN("command");

// ---------------------------------------------------------------- TaskStateCommand

TEST_CASE_FIXTURE(CommandTest, "start and complete follow the state pattern") {
    Task* t = addTask("Review");
    act(t, "start");
    CHECK(state(t) == "In Progress");
    act(t, "complete");
    CHECK(state(t) == "Complete");
    CHECK(t->isCompleted());
}

TEST_CASE_FIXTURE(CommandTest, "undo and redo restore lifecycle state") {
    Task* t = addTask("Review");
    act(t, "start");
    act(t, "complete");

    history.undo();
    CHECK(state(t) == "In Progress");
    history.redo();
    CHECK(state(t) == "Complete");
}

TEST_CASE_FIXTURE(CommandTest, "undoing completion makes dependent not ready again") {
    Task* a = addTask("A");
    Task* b = addTask("B");
    run(new AddDependencyCommand(b, a));
    act(a, "start");
    act(a, "complete");
    CHECK(b->isReady());

    history.undo();
    CHECK_FALSE(b->isReady());
}

TEST_CASE_FIXTURE(CommandTest, "illegal transition is rejected without changes") {
    Task* t = addTask("Review");
    CHECK_THROWS_AS(act(t, "complete"), InvalidTransitionError);
    CHECK(state(t) == "Pending");

    history.undo();
    CHECK(project.getWorkItems().empty());
}

TEST_CASE_FIXTURE(CommandTest, "start is rejected until prerequisites are complete") {
    Task* a = addTask("A");
    Task* b = addTask("B");
    run(new AddDependencyCommand(b, a));

    CHECK_THROWS_AS(act(b, "start"), InvalidTransitionError);
    CHECK(state(b) == "Pending");
}

TEST_CASE_FIXTURE(CommandTest, "block and cancel from in progress") {
    Task* a = addTask("A");
    Task* b = addTask("B");
    act(a, "start");
    act(a, "block");
    CHECK(state(a) == "Blocked");
    act(b, "start");
    act(b, "cancel");
    CHECK(state(b) == "Cancelled");
}

TEST_CASE_FIXTURE(CommandTest, "fail then retry") {
    Task* t = addTask("Payment");
    act(t, "start");
    act(t, "fail");
    CHECK(state(t) == "Failed");
    act(t, "start");
    CHECK(state(t) == "In Progress");
}

// ---------------------------------------------------------------- ActionRegistry

TEST_CASE_FIXTURE(CommandTest, "registry has built in lifecycle actions") {
    CHECK(actions.has("start"));
    CHECK(actions.has("complete"));
    CHECK(actions.has("block"));
    CHECK(actions.has("fail"));
    CHECK(actions.has("cancel"));
    CHECK_FALSE(actions.has("approve"));
}

TEST_CASE_FIXTURE(CommandTest, "unknown action throws not found") {
    Task* t = addTask("Review");
    CHECK_THROWS_AS(actions.create("teleport", t, nlohmann::json()), NotFoundError);
}

TEST_CASE_FIXTURE(CommandTest, "industry action can be registered") {
    actions.registerAction("approve", [](Task* task, const nlohmann::json&) -> Command* {
        return new TaskStateCommand(task, [](Task& t) { t.complete(); }, "Approve");
    });
    Task* t = addTask("Claim review");
    act(t, "start");
    act(t, "approve");
    CHECK(state(t) == "Complete");
}

// ---------------------------------------------------------------- AddWorkItemCommand

TEST_CASE_FIXTURE(CommandTest, "add work item undo and redo") {
    Task* t = addTask("Review");
    CHECK(project.getWorkItems().size() == 1u);

    history.undo();
    CHECK(project.getWorkItems().empty());
    history.redo();
    REQUIRE(project.getWorkItems().size() == 1u);
    CHECK(project.getWorkItems()[0] == t);
}

TEST_CASE_FIXTURE(CommandTest, "undone add is freed when redo is discarded") {
    addTask("Temporary");
    history.undo();
    addTask("Replacement");
    CHECK_FALSE(history.canRedo());
    CHECK(project.getWorkItems().size() == 1u);
}

// ---------------------------------------------------------------- AddDependencyCommand

TEST_CASE_FIXTURE(CommandTest, "add dependency undo removes the link") {
    Task* a = addTask("A");
    Task* b = addTask("B");
    run(new AddDependencyCommand(b, a));
    CHECK_FALSE(b->isReady());

    history.undo();
    CHECK(b->getDependencies().empty());
    CHECK(b->isReady());
}

TEST_CASE_FIXTURE(CommandTest, "cyclic dependency is rejected without changes") {
    Task* a = addTask("A");
    Task* b = addTask("B");
    run(new AddDependencyCommand(b, a));

    CHECK_THROWS_AS(run(new AddDependencyCommand(a, b)), CyclicDependencyError);
    CHECK(a->getDependencies().empty());
}

// ---------------------------------------------------------------- EditTaskInfoCommand (Memento)

TEST_CASE_FIXTURE(CommandTest, "edit task undo and redo") {
    Task* t = addTask("Review");
    nlohmann::json payload = {{"name", "Review claim"},
                              {"description", "Check documents"},
                              {"customFields", {{"priority", "high"}}}};

    run(new EditTaskInfoCommand(t, payload));
    CHECK(t->getName() == "Review claim");
    CHECK(t->getCustomField("priority") == "high");

    history.undo();
    CHECK(t->getName() == "Review");
    CHECK(t->getDescription() == "");

    history.redo();
    CHECK(t->getName() == "Review claim");
    CHECK(t->getDescription() == "Check documents");
}

TEST_CASE_FIXTURE(CommandTest, "edit assigns executor") {
    Task* t = addTask("Assess claim");
    nlohmann::json payload = {{"executor", {{"id", "u7"}, {"name", "Thandi"}, {"role", "Assessor"}}}};

    run(new EditTaskInfoCommand(t, payload));
    CHECK(t->getExecutorId() == "u7");
    history.undo();
    CHECK(t->getExecutorId() == "");
}

TEST_CASE_FIXTURE(CommandTest, "edit with unknown field changes nothing") {
    Task* t = addTask("Review");
    nlohmann::json payload = {{"name", "New"}, {"colour", "blue"}};

    CHECK_THROWS_AS(run(new EditTaskInfoCommand(t, payload)), WorkflowError);
    CHECK(t->getName() == "Review");
}

TEST_CASE_FIXTURE(CommandTest, "edit with malformed value is a workflow error") {
    Task* t = addTask("Review");
    nlohmann::json payload = {{"deadline", {{"year", "soon"}}}};
    CHECK_THROWS_AS(run(new EditTaskInfoCommand(t, payload)), WorkflowError);
}

// ---------------------------------------------------------------- DecideCommand

TEST_CASE_FIXTURE(CommandTest, "decision skips other branches and completes decision") {
    DecisionTask* d = addDecision("Route claim");
    Task* approve = addTask("Pay out");
    Task* reject = addTask("Send rejection");
    d->addBranchTarget("approve", approve);
    d->addBranchTarget("reject", reject);
    act(d, "start");

    run(new DecideCommand(d, {{"outcome", "approve"}}));
    CHECK(d->getSelectedBranch() == "approve");
    CHECK(state(d) == "Complete");
    CHECK(state(reject) == "Skipped");
    CHECK(state(approve) == "Pending");
    CHECK(approve->isReady());
}

TEST_CASE_FIXTURE(CommandTest, "undo decision restores everything") {
    DecisionTask* d = addDecision("Route claim");
    Task* approve = addTask("Pay out");
    Task* reject = addTask("Send rejection");
    d->addBranchTarget("approve", approve);
    d->addBranchTarget("reject", reject);
    act(d, "start");
    run(new DecideCommand(d, {{"outcome", "approve"}}));

    history.undo();
    CHECK(d->getSelectedBranch() == "");
    CHECK(state(d) == "In Progress");
    CHECK(state(reject) == "Pending");
    CHECK_FALSE(approve->isReady());

    history.redo();
    CHECK(state(reject) == "Skipped");
}

TEST_CASE_FIXTURE(CommandTest, "decision with unknown branch changes nothing") {
    DecisionTask* d = addDecision("Route claim");
    d->addBranchTarget("approve", addTask("Pay out"));
    act(d, "start");

    CHECK_THROWS_AS(run(new DecideCommand(d, {{"outcome", "maybe"}})), NotFoundError);
    CHECK(state(d) == "In Progress");
    CHECK(d->getSelectedBranch() == "");
}

TEST_CASE_FIXTURE(CommandTest, "decision before start is rejected") {
    DecisionTask* d = addDecision("Route claim");
    Task* reject = addTask("Send rejection");
    d->addBranchTarget("approve", addTask("Pay out"));
    d->addBranchTarget("reject", reject);

    CHECK_THROWS_AS(run(new DecideCommand(d, {{"outcome", "approve"}})), InvalidTransitionError);
    CHECK(state(reject) == "Pending");
}

TEST_CASE_FIXTURE(CommandTest, "second decision is rejected until undone") {
    DecisionTask* d = addDecision("Route claim");
    d->addBranchTarget("approve", addTask("Pay out"));
    d->addBranchTarget("reject", addTask("Send rejection"));
    act(d, "start");
    run(new DecideCommand(d, {{"outcome", "approve"}}));
    act(d, "start");

    CHECK_THROWS_AS(run(new DecideCommand(d, {{"outcome", "reject"}})), InvalidTransitionError);
    CHECK(d->getSelectedBranch() == "approve");
}

// ---------------------------------------------------------------- CommandHistory

TEST_CASE_FIXTURE(CommandTest, "undo and redo on empty history throw") {
    CommandHistory empty;
    CHECK_FALSE(empty.canUndo());
    CHECK_FALSE(empty.canRedo());
    CHECK_THROWS_AS(empty.undo(), WorkflowError);
    CHECK_THROWS_AS(empty.redo(), WorkflowError);
}

TEST_CASE_FIXTURE(CommandTest, "new action clears redo") {
    Task* t = addTask("Review");
    act(t, "start");
    history.undo();
    CHECK(history.canRedo());

    act(t, "start");
    CHECK_FALSE(history.canRedo());
}

TEST_CASE_FIXTURE(CommandTest, "describe names the action") {
    Task* t = addTask("Review");
    Command* cmd = actions.create("start", t, nlohmann::json());
    CHECK(cmd->describe().find(t->getId()) != std::string::npos);
    delete cmd;
}

TEST_SUITE_END();
