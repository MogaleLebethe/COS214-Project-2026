/**
 * @file test_observer.cpp
 * @brief Observer pattern: dependency registration, cycle rejection, readiness cascade.
 */
#include <gtest/gtest.h>

#include <stdexcept>

#include "TestHelpers.h"
#include "common/Errors.h"

using namespace testutil;

namespace {

class ObserverTest : public ::testing::Test {
protected:
    Project project{"Observer project"};
};

}  // namespace

TEST_F(ObserverTest, AddDependencyLinksBothDirections) {
    Task* a = addTask(project, "A");
    Task* b = addTask(project, "B");
    EXPECT_TRUE(b->addDependency(a));
    ASSERT_EQ(1u, b->getDependencies().size());
    EXPECT_EQ(a, b->getDependencies()[0]);
    ASSERT_EQ(1u, a->getDependents().size());
    EXPECT_EQ(b, a->getDependents()[0]);
}

TEST_F(ObserverTest, DuplicateLinkReturnsFalseAndDoesNotDuplicate) {
    Task* a = addTask(project, "A");
    Task* b = addTask(project, "B");
    EXPECT_TRUE(b->addDependency(a));
    EXPECT_FALSE(b->addDependency(a));
    EXPECT_EQ(1u, b->getDependencies().size());
    EXPECT_EQ(1u, a->getDependents().size());
}

TEST_F(ObserverTest, NullPrerequisiteIsProgrammerError) {
    Task* a = addTask(project, "A");
    EXPECT_THROW(a->addDependency(0), std::logic_error);
}

TEST_F(ObserverTest, SelfDependencyIsRejected) {
    Task* a = addTask(project, "A");
    EXPECT_THROW(a->addDependency(a), CyclicDependencyError);
    EXPECT_TRUE(a->getDependencies().empty());
}

TEST_F(ObserverTest, DirectCycleIsRejectedWithoutMutation) {
    Task* a = addTask(project, "A");
    Task* b = addTask(project, "B");
    b->addDependency(a);
    EXPECT_THROW(a->addDependency(b), CyclicDependencyError);
    EXPECT_TRUE(a->getDependencies().empty());
    EXPECT_TRUE(b->getDependents().empty());
}

TEST_F(ObserverTest, TransitiveCycleIsRejected) {
    Task* a = addTask(project, "A");
    Task* b = addTask(project, "B");
    Task* c = addTask(project, "C");
    b->addDependency(a);
    c->addDependency(b);
    EXPECT_THROW(a->addDependency(c), CyclicDependencyError);
}

TEST_F(ObserverTest, CyclicDependencyErrorIsAWorkflowError) {
    Task* a = addTask(project, "A");
    EXPECT_THROW(a->addDependency(a), WorkflowError);
}

TEST_F(ObserverTest, ChildCannotDependOnItsOwnStage) {
    Project* stage = addStage(project, "Stage");
    Task* inner = addTask(*stage, "Inner");
    EXPECT_THROW(inner->addDependency(stage), CyclicDependencyError);
}

TEST_F(ObserverTest, StageCannotDependOnItsOwnChild) {
    Project* stage = addStage(project, "Stage");
    Task* inner = addTask(*stage, "Inner");
    EXPECT_THROW(stage->addDependency(inner), CyclicDependencyError);
}

TEST_F(ObserverTest, CycleThroughStageMembershipIsRejected) {
    // outer waits for Stage; Stage's child "inner" waits for outer -> cycle via containment.
    Task* outer = addTask(project, "Outer");
    Project* stage = addStage(project, "Stage");
    Task* inner = addTask(*stage, "Inner");
    outer->addDependency(stage);
    EXPECT_THROW(inner->addDependency(outer), CyclicDependencyError);
}

TEST_F(ObserverTest, CycleThroughEnclosingStageDependencyIsRejected) {
    // Stage waits for A, so Stage's child waits for A too; A may not wait for that child.
    Task* a = addTask(project, "A");
    Project* stage = addStage(project, "Stage");
    Task* inner = addTask(*stage, "Inner");
    stage->addDependency(a);
    EXPECT_THROW(a->addDependency(inner), CyclicDependencyError);
}

TEST_F(ObserverTest, ComponentWithoutPrerequisitesIsReady) {
    Task* a = addTask(project, "A");
    EXPECT_TRUE(a->isReady());
}

TEST_F(ObserverTest, DependentBecomesReadyWhenPrerequisiteCompletes) {
    Task* a = addTask(project, "A");
    Task* b = addTask(project, "B");
    b->addDependency(a);
    EXPECT_FALSE(b->isReady());
    a->start();
    EXPECT_FALSE(b->isReady());
    a->complete();
    EXPECT_TRUE(b->isReady());
}

TEST_F(ObserverTest, ReadinessRequiresEveryPrerequisite) {
    Task* a = addTask(project, "A");
    Task* b = addTask(project, "B");
    Task* c = addTask(project, "C");
    c->addDependency(a);
    c->addDependency(b);
    finish(a);
    EXPECT_FALSE(c->isReady());
    finish(b);
    EXPECT_TRUE(c->isReady());
}

TEST_F(ObserverTest, LinkingToCompletedPrerequisiteIsImmediatelyReady) {
    Task* a = addTask(project, "A");
    Task* b = addTask(project, "B");
    finish(a);
    b->addDependency(a);
    EXPECT_TRUE(b->isReady());
}

TEST_F(ObserverTest, EveryStateChangeNotifiesDependents) {
    Task* a = addTask(project, "A");
    Task* b = addTask(project, "B");
    Task* c = addTask(project, "C");
    b->addDependency(a);
    c->addDependency(a);
    int before = b->getNotificationCount();
    a->start();
    a->complete();
    EXPECT_EQ(before + 2, b->getNotificationCount());
    EXPECT_EQ(before + 2, c->getNotificationCount());
}

TEST_F(ObserverTest, ReopeningPrerequisiteRevokesReadiness) {
    Task* a = addTask(project, "A");
    Task* b = addTask(project, "B");
    b->addDependency(a);
    finish(a);
    ASSERT_TRUE(b->isReady());
    a->start();  // Complete -> In Progress (re-open)
    EXPECT_FALSE(b->isReady());
}

TEST_F(ObserverTest, SetStateUsedByUndoReNotifiesDependents) {
    Task* a = addTask(project, "A");
    Task* b = addTask(project, "B");
    b->addDependency(a);
    a->start();
    TaskState* before = a->getState();
    a->complete();
    ASSERT_TRUE(b->isReady());
    a->setState(before);  // what TaskStateCommand::undo does
    EXPECT_FALSE(b->isReady());
}

TEST_F(ObserverTest, SkippedPrerequisiteCountsAsSatisfied) {
    Task* a = addTask(project, "A");
    Task* b = addTask(project, "B");
    b->addDependency(a);
    a->skip();
    EXPECT_TRUE(b->isReady());
}

TEST_F(ObserverTest, CancelledOrFailedPrerequisiteKeepsDependentWaiting) {
    Task* a = addTask(project, "A");
    Task* b = addTask(project, "B");
    b->addDependency(a);
    a->start();
    a->fail();
    EXPECT_FALSE(b->isReady());
    a->cancel();
    EXPECT_FALSE(b->isReady());
}

TEST_F(ObserverTest, ObserversNeverChangeTaskState) {
    Task* a = addTask(project, "A");
    Task* b = addTask(project, "B");
    b->addDependency(a);
    finish(a);
    EXPECT_EQ("Pending", stateOf(b));  // ready, but nobody started it
}

TEST_F(ObserverTest, CascadeIsOneLevelOfReadinessAtATime) {
    Task* a = addTask(project, "A");
    Task* b = addTask(project, "B");
    Task* c = addTask(project, "C");
    b->addDependency(a);
    c->addDependency(b);
    finish(a);
    EXPECT_TRUE(b->isReady());
    EXPECT_FALSE(c->isReady());
    finish(b);
    EXPECT_TRUE(c->isReady());
}

TEST_F(ObserverTest, RemoveDependencyUnlinksBothSidesAndReevaluates) {
    Task* a = addTask(project, "A");
    Task* b = addTask(project, "B");
    b->addDependency(a);
    ASSERT_FALSE(b->isReady());
    b->removeDependency(a);
    EXPECT_TRUE(b->getDependencies().empty());
    EXPECT_TRUE(a->getDependents().empty());
    EXPECT_TRUE(b->isReady());
}

TEST_F(ObserverTest, RemovingUnknownDependencyIsANoOp) {
    Task* a = addTask(project, "A");
    Task* b = addTask(project, "B");
    b->removeDependency(a);
    b->removeDependency(0);
    EXPECT_TRUE(b->getDependencies().empty());
}

TEST_F(ObserverTest, UnlinkAllRemovesEveryEdgeAndReleasesDependents) {
    Task* a = addTask(project, "A");
    Task* b = addTask(project, "B");
    Task* c = addTask(project, "C");
    b->addDependency(a);
    c->addDependency(b);
    EXPECT_TRUE(b->unlinkAll());
    EXPECT_TRUE(b->getDependencies().empty());
    EXPECT_TRUE(b->getDependents().empty());
    EXPECT_TRUE(a->getDependents().empty());
    EXPECT_TRUE(c->getDependencies().empty());
    EXPECT_TRUE(c->isReady());
    EXPECT_FALSE(b->unlinkAll());  // nothing left to remove
}

TEST_F(ObserverTest, ProjectUnlinkAllAppliesToDescendants) {
    Task* a = addTask(project, "A");
    Project* stage = addStage(project, "Stage");
    Task* inner = addTask(*stage, "Inner");
    inner->addDependency(a);
    EXPECT_TRUE(stage->unlinkAll());
    EXPECT_TRUE(inner->getDependencies().empty());
    EXPECT_TRUE(a->getDependents().empty());
}

TEST_F(ObserverTest, RemoveWorkItemUnlinksTheRemovedComponent) {
    Task* a = addTask(project, "A");
    Task* b = addTask(project, "B");
    b->addDependency(a);
    WorkflowComponent* removed = project.removeWorkItem(a);
    EXPECT_TRUE(b->getDependencies().empty());
    EXPECT_TRUE(b->isReady());
    delete removed;  // ownership was transferred to us
}

TEST_F(ObserverTest, TaskInsideWaitingStageIsNotReady) {
    Task* a = addTask(project, "A");
    Project* stage = addStage(project, "Stage");
    Task* inner = addTask(*stage, "Inner");
    stage->addDependency(a);
    EXPECT_FALSE(stage->isReady());
    EXPECT_FALSE(inner->isReady());
    EXPECT_THROW(inner->start(), InvalidTransitionError);
    finish(a);
    EXPECT_TRUE(inner->isReady());
    EXPECT_NO_THROW(inner->start());
}

TEST_F(ObserverTest, StageCompletionNotifiesTheStagesDependents) {
    Project* stage = addStage(project, "Stage");
    Task* inner1 = addTask(*stage, "Inner 1");
    Task* inner2 = addTask(*stage, "Inner 2");
    Task* after = addTask(project, "After");
    after->addDependency(stage);
    finish(inner1);
    EXPECT_FALSE(after->isReady());
    finish(inner2);
    EXPECT_TRUE(stage->isCompleted());
    EXPECT_TRUE(after->isReady());
}

TEST_F(ObserverTest, AddingWorkToCompletedStageRevokesDependentsReadiness) {
    Project* stage = addStage(project, "Stage");
    Task* after = addTask(project, "After");
    after->addDependency(stage);
    EXPECT_TRUE(after->isReady());  // empty stage counts as complete
    addTask(*stage, "Late work");
    EXPECT_FALSE(after->isReady());
}

TEST_F(ObserverTest, RemovingLastOpenItemFromStageReleasesDependents) {
    Project* stage = addStage(project, "Stage");
    Task* open = addTask(*stage, "Open");
    Task* after = addTask(project, "After");
    after->addDependency(stage);
    ASSERT_FALSE(after->isReady());
    delete stage->removeWorkItem(open);
    EXPECT_TRUE(after->isReady());
}

TEST_F(ObserverTest, CrossStageDependencyIsTracked) {
    Project* s1 = addStage(project, "Stage 1");
    Project* s2 = addStage(project, "Stage 2");
    Task* a = addTask(*s1, "A");
    Task* b = addTask(*s2, "B");
    b->addDependency(a);
    EXPECT_FALSE(b->isReady());
    finish(a);
    EXPECT_TRUE(b->isReady());
}

TEST_F(ObserverTest, DefaultUpdateCountsAndReevaluates) {
    Task* a = addTask(project, "A");
    Task* b = addTask(project, "B");
    b->addDependency(a);
    int before = b->getNotificationCount();
    b->update(a);
    EXPECT_EQ(before + 1, b->getNotificationCount());
    EXPECT_FALSE(b->isReady());
}

TEST_F(ObserverTest, ExportedJsonListsDependenciesAndReadiness) {
    Task* a = addTask(project, "A");
    Task* b = addTask(project, "B");
    b->addDependency(a);
    nlohmann::json j = b->toJson();
    ASSERT_EQ(1u, j["dependsOn"].size());
    EXPECT_EQ(a->getId(), j["dependsOn"][0].get<std::string>());
    EXPECT_FALSE(j["ready"].get<bool>());
}
