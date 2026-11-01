/**
 * @file test_prototype.cpp
 * @brief Prototype pattern: clone() of tasks, decisions and whole projects.
 */
#include <gtest/gtest.h>

#include <memory>

#include "HealthcareModule.h"
#include "InsuranceModule.h"
#include "TestHelpers.h"
#include "builder/WorkflowFactory.h"
#include "common/Errors.h"
#include "facade/WorkflowEngineFacade.h"
#include "routing/RoutingStrategy.h"

using namespace testutil;

namespace {

/** @brief Owns a raw clone for the duration of a test (tests only). */
template <typename T>
struct Owned {
    T* ptr;
    explicit Owned(T* p) : ptr(p) {}
    ~Owned() { delete ptr; }
    T* operator->() const { return ptr; }
    T& operator*() const { return *ptr; }

private:
    Owned(const Owned&);
    Owned& operator=(const Owned&);
};

Task* childNamed(const Project& p, const std::string& name) {
    for (std::size_t i = 0; i < p.getWorkItems().size(); ++i) {
        if (p.getWorkItems()[i]->getName() == name) return dynamic_cast<Task*>(p.getWorkItems()[i]);
    }
    return 0;
}

}  // namespace

// ---------------------------------------------------------------- Task

TEST(PrototypeTask, CloneHasFreshIdAndCopiedAttributes) {
    Task original("Review", "Check the documents");
    original.setExecutor(Participant("E1", "Eve", "Reviewer"));
    original.addParticipant(Participant("W1", "Walt", "Watcher"));
    original.setDeadline(DateTime(2026, 11, 2, 9, 30));
    original.setCustomField("priority", "high");
    Owned<Task> copy(original.clone());
    EXPECT_NE(original.getId(), copy->getId());
    EXPECT_FALSE(copy->getId().empty());
    EXPECT_EQ("Review", copy->getName());
    EXPECT_EQ("Check the documents", copy->getDescription());
    EXPECT_EQ("E1", copy->getExecutorId());
    EXPECT_EQ("Eve", copy->getExecutor().getName());
    EXPECT_EQ(2u, copy->getParticipants().size());
    EXPECT_EQ(DateTime(2026, 11, 2, 9, 30), copy->getDeadline());
    EXPECT_EQ("high", copy->getCustomField("priority"));
}

TEST(PrototypeTask, CloneResetsRuntimeData) {
    Task original("Work");
    original.addComment(Comment("u1", "started early"));
    original.start();
    Owned<Task> copy(original.clone());
    EXPECT_EQ("Pending", stateOf(copy.ptr));
    EXPECT_TRUE(copy->getComments().empty());
    EXPECT_EQ("In Progress", stateOf(&original));
    EXPECT_EQ(1u, original.getComments().size());
}

TEST(PrototypeTask, CloneHasNoEdgesAndNoParent) {
    Project project("P");
    Task* a = addTask(project, "A");
    Task* b = addTask(project, "B");
    b->addDependency(a);
    Owned<Task> copy(b->clone());
    EXPECT_TRUE(copy->getDependencies().empty());
    EXPECT_TRUE(copy->getDependents().empty());
    EXPECT_EQ(0, copy->getParent());
    EXPECT_TRUE(copy->isReady());
    EXPECT_EQ(1u, a->getDependents().size());  // the original's peer is untouched
}

TEST(PrototypeTask, CloneIsIndependentOfOriginal) {
    Task original("Original");
    Owned<Task> copy(original.clone());
    copy->setName("Changed");
    copy->setCustomField("k", "v");
    copy->start();
    EXPECT_EQ("Original", original.getName());
    EXPECT_FALSE(original.hasCustomField("k"));
    EXPECT_EQ("Pending", stateOf(&original));
}

TEST(PrototypeTask, CloneThroughBasePointerKeepsDynamicType) {
    DecisionTask decision("Route");
    const WorkflowComponent& base = decision;
    Owned<WorkflowComponent> copy(base.clone());
    EXPECT_NE(static_cast<DecisionTask*>(0), dynamic_cast<DecisionTask*>(copy.ptr));
    EXPECT_EQ("Decision", copy->toJson()["type"].get<std::string>());
}

// ---------------------------------------------------------------- DecisionTask

TEST(PrototypeDecision, CloneKeepsRouterButNotSelectionOrBranches) {
    FieldValueRoutingStrategy router("outcome");
    Project project("P");
    DecisionTask* d = addDecision(project, "Route");
    Task* yes = addTask(project, "Yes");
    d->setRouter(&router);
    d->addBranchTarget("yes", yes);
    d->decide(nlohmann::json::parse("{\"outcome\":\"yes\"}"));
    Owned<DecisionTask> copy(d->clone());
    EXPECT_EQ(&router, copy->getRouter());
    EXPECT_TRUE(copy->getSelectedBranch().empty());
    EXPECT_TRUE(copy->getBranchTargets().empty());
    EXPECT_EQ("yes", d->getSelectedBranch());
}

// ---------------------------------------------------------------- Project

TEST(PrototypeProject, DeepCloneCopiesEveryChildWithFreshIds) {
    Project original("Original", "desc");
    original.setCustomField("industry", "Test");
    addTask(original, "A");
    Project* stage = addStage(original, "Stage");
    addTask(*stage, "Inner");
    Owned<Project> copy(original.clone());
    EXPECT_NE(original.getId(), copy->getId());
    EXPECT_EQ("Original", copy->getName());
    EXPECT_EQ("desc", copy->getDescription());
    EXPECT_EQ("Test", copy->getCustomField("industry"));
    ASSERT_EQ(2u, copy->getWorkItems().size());
    for (std::size_t i = 0; i < 2; ++i) {
        EXPECT_NE(original.getWorkItems()[i], copy->getWorkItems()[i]);
        EXPECT_NE(original.getWorkItems()[i]->getId(), copy->getWorkItems()[i]->getId());
        EXPECT_EQ(original.getWorkItems()[i]->getName(), copy->getWorkItems()[i]->getName());
        EXPECT_EQ(copy.ptr, copy->getWorkItems()[i]->getParent());
    }
    const Project* copiedStage = dynamic_cast<const Project*>(copy->getWorkItems()[1]);
    ASSERT_NE(static_cast<const Project*>(0), copiedStage);
    ASSERT_EQ(1u, copiedStage->getWorkItems().size());
    EXPECT_EQ("Inner", copiedStage->getWorkItems()[0]->getName());
    EXPECT_EQ(0, original.findById(copiedStage->getWorkItems()[0]->getId()));
}

TEST(PrototypeProject, InternalDependencyEdgesAreRemappedToTheCopies) {
    Project original("P");
    Task* a = addTask(original, "A");
    Task* b = addTask(original, "B");
    b->addDependency(a);
    Owned<Project> copy(original.clone());
    Task* ca = childNamed(*copy, "A");
    Task* cb = childNamed(*copy, "B");
    ASSERT_EQ(1u, cb->getDependencies().size());
    EXPECT_EQ(ca, cb->getDependencies()[0]);
    EXPECT_EQ(1u, a->getDependents().size());  // original A still has only original B
    EXPECT_EQ(b, a->getDependents()[0]);
    EXPECT_FALSE(cb->isReady());
}

TEST(PrototypeProject, EdgesLeavingTheClonedSubtreeAreDropped) {
    Project root("Root");
    Task* outside = addTask(root, "Outside");
    Project* stage = addStage(root, "Stage");
    Task* inner = addTask(*stage, "Inner");
    inner->addDependency(outside);
    stage->addDependency(outside);
    Owned<Project> copy(stage->clone());
    EXPECT_TRUE(copy->getDependencies().empty());
    EXPECT_TRUE(copy->getWorkItems()[0]->getDependencies().empty());
    EXPECT_EQ(2u, outside->getDependents().size());
}

TEST(PrototypeProject, EdgesAcrossNestedStagesAreRemapped) {
    Project root("Root");
    Task* first = addTask(root, "First");
    Project* stage = addStage(root, "Stage");
    Task* inner = addTask(*stage, "Inner");
    stage->addDependency(first);
    inner->addDependency(first);
    Owned<Project> copy(root.clone());
    Task* copiedFirst = childNamed(*copy, "First");
    Project* copiedStage = dynamic_cast<Project*>(copy->getWorkItems()[1]);
    ASSERT_NE(static_cast<Project*>(0), copiedStage);
    ASSERT_EQ(1u, copiedStage->getDependencies().size());
    EXPECT_EQ(copiedFirst, copiedStage->getDependencies()[0]);
    ASSERT_EQ(1u, copiedStage->getWorkItems()[0]->getDependencies().size());
    EXPECT_EQ(copiedFirst, copiedStage->getWorkItems()[0]->getDependencies()[0]);
    EXPECT_EQ(2u, copiedFirst->getDependents().size());
}

TEST(PrototypeProject, BranchTargetsAreRemapped) {
    FieldValueRoutingStrategy router("outcome");
    Project original("P");
    DecisionTask* d = addDecision(original, "Route");
    Task* yes = addTask(original, "Yes");
    Task* no = addTask(original, "No");
    d->setRouter(&router);
    d->addBranchTarget("yes", yes);
    d->addBranchTarget("no", no);
    Owned<Project> copy(original.clone());
    DecisionTask* cd = dynamic_cast<DecisionTask*>(copy->getWorkItems()[0]);
    ASSERT_NE(static_cast<DecisionTask*>(0), cd);
    ASSERT_EQ(2u, cd->getBranchTargets().size());
    EXPECT_EQ(childNamed(*copy, "Yes"), cd->getBranchTargets().find("yes")->second[0]);
    EXPECT_EQ(childNamed(*copy, "No"), cd->getBranchTargets().find("no")->second[0]);
    // Branch targets depend on the copied decision, not on the original.
    EXPECT_EQ(cd, childNamed(*copy, "Yes")->getDependencies()[0]);
    EXPECT_EQ(1u, childNamed(*copy, "Yes")->getDependencies().size());
}

TEST(PrototypeProject, CloneResetsEveryStateToPending) {
    Project original("P");
    Task* a = addTask(original, "A");
    Task* b = addTask(original, "B");
    b->addDependency(a);
    finish(a);
    b->start();
    Owned<Project> copy(original.clone());
    EXPECT_EQ("Pending", stateOf(childNamed(*copy, "A")));
    EXPECT_EQ("Pending", stateOf(childNamed(*copy, "B")));
    EXPECT_FALSE(childNamed(*copy, "B")->isReady());
    EXPECT_FALSE(copy->isCompleted());
}

TEST(PrototypeProject, ProgressInCloneDoesNotAffectOriginal) {
    Project original("P");
    Task* a = addTask(original, "A");
    Task* b = addTask(original, "B");
    b->addDependency(a);
    Owned<Project> copy(original.clone());
    finish(childNamed(*copy, "A"));
    EXPECT_TRUE(childNamed(*copy, "B")->isReady());
    EXPECT_EQ("Pending", stateOf(a));
    EXPECT_FALSE(b->isReady());
}

TEST(PrototypeProject, CloneOfEmptyProjectIsEmptyAndCompleted) {
    Project original("Empty");
    Owned<Project> copy(original.clone());
    EXPECT_TRUE(copy->getWorkItems().empty());
    EXPECT_TRUE(copy->isCompleted());
}

TEST(PrototypeProject, CloneOfCloneIsStillIndependent) {
    Project original("P");
    Task* a = addTask(original, "A");
    addTask(original, "B")->addDependency(a);
    Owned<Project> first(original.clone());
    Owned<Project> second(first->clone());
    EXPECT_NE(first->getId(), second->getId());
    EXPECT_EQ(childNamed(*second, "A"), childNamed(*second, "B")->getDependencies()[0]);
}

TEST(PrototypeProject, ClonedRoutingWorkflowRunsIndependently) {
    FieldValueRoutingStrategy router("outcome");
    Project original("P");
    DecisionTask* d = addDecision(original, "Route");
    d->setRouter(&router);
    d->addBranchTarget("yes", addTask(original, "Yes"));
    Owned<Project> copy(original.clone());
    DecisionTask* cd = dynamic_cast<DecisionTask*>(copy->getWorkItems()[0]);
    EXPECT_EQ("yes", cd->decide(nlohmann::json::parse("{\"outcome\":\"yes\"}")));
    EXPECT_TRUE(d->getSelectedBranch().empty());
}

// ---------------------------------------------------------------- Factory / Facade

TEST(PrototypeFactory, DuplicateRenamesAndRecordsSource) {
    WorkflowFactory factory;
    Project source("Source");
    addTask(source, "A");
    Owned<Project> copy(factory.duplicate(source, "Copy"));
    EXPECT_EQ("Copy", copy->getName());
    EXPECT_EQ(source.getId(), copy->getCustomField("clonedFrom"));
    EXPECT_EQ(1u, copy->getWorkItems().size());
}

TEST(PrototypeFactory, DuplicateWithEmptyNameKeepsOriginalName) {
    WorkflowFactory factory;
    Project source("Source");
    Owned<Project> copy(factory.duplicate(source, ""));
    EXPECT_EQ("Source", copy->getName());
}

TEST(PrototypeFacade, CloneProjectCreatesIsolatedInstanceWithOwnHistory) {
    WorkflowEngineFacade engine;
    InsuranceModule insurance;
    engine.installModule(insurance);
    std::string original = engine.startInstance(InsuranceModule::kMotorClaimDefinition, "Claim A");
    std::string firstTask = engine.listAvailableWork(original)[0].id;
    engine.performAction(original, firstTask, "start");

    std::string copy = engine.cloneProject(original, "Claim B");
    EXPECT_NE(original, copy);
    EXPECT_EQ("Claim B", engine.exportProjectToJson(copy)["name"].get<std::string>());
    EXPECT_FALSE(engine.canUndo(copy));  // history is per project
    EXPECT_TRUE(engine.canUndo(original));
    std::vector<WorkItemView> work = engine.listAvailableWork(copy);
    ASSERT_EQ(1u, work.size());
    EXPECT_EQ("Pending", work[0].status);
    EXPECT_NE(firstTask, work[0].id);
    EXPECT_EQ(2u, engine.listProjects().size());
}

TEST(PrototypeFacade, CloneOfUnknownProjectThrowsNotFound) {
    WorkflowEngineFacade engine;
    EXPECT_THROW(engine.cloneProject("P-missing", "x"), NotFoundError);
}

TEST(PrototypeFacade, ClonedTemplateInstanceKeepsRoutingAndStages) {
    WorkflowEngineFacade engine;
    HealthcareModule healthcare;
    engine.installModule(healthcare);
    std::string original = engine.startInstance(HealthcareModule::kAdmissionDefinition, "Jane");
    std::string copy = engine.cloneProject(original, "John");
    nlohmann::json j = engine.exportProjectToJson(copy);
    const nlohmann::json& items = j["workItems"];
    ASSERT_EQ(8u, items.size());
    EXPECT_EQ("Decision", items[3]["type"].get<std::string>());
    EXPECT_EQ(3u, items[3]["branches"].size());
    EXPECT_EQ(4u, items[7]["dependsOn"].size());          // Discharge stage edges survived
    EXPECT_FALSE(items[7]["ready"].get<bool>());
}

// ---------------------------------------------------------------- failure paths

namespace {

/** @brief A component whose clone() misbehaves, to exercise Project::clone()'s cleanup. */
class FaultyComponent : public WorkflowComponent {
public:
    explicit FaultyComponent(bool throwOnClone) : WorkflowComponent("F1", "Faulty", ""), throwOnClone(throwOnClone) {}
    void evaluateDependencies() { recomputeReadiness(); }
    std::string getStatusSummary() const { return "Faulty"; }
    bool isCompleted() const { return false; }
    void collectWork(std::vector<WorkItemView>&) const {}
    nlohmann::json toJson() const { return baseJson("Faulty"); }
    WorkflowComponent* clone() const {
        if (throwOnClone) throw WorkflowError("cannot copy this component");
        return 0;  // a broken prototype: addWorkItem rejects the null copy
    }

private:
    bool throwOnClone;
};

}  // namespace

TEST(PrototypeFailure, ThrowingChildCloneDeletesThePartialCopy) {
    Project original("P");
    addTask(original, "Copied before the failure");
    original.addWorkItem(new FaultyComponent(true));
    EXPECT_THROW(original.clone(), WorkflowError);  // ASan/Valgrind: the partial copy is freed
    EXPECT_EQ(2u, original.getWorkItems().size());
}

TEST(PrototypeFailure, RejectedChildCopyIsDeletedAndPartialCopyFreed) {
    Project original("P");
    addTask(original, "A");
    original.addWorkItem(new FaultyComponent(false));
    EXPECT_THROW(original.clone(), WorkflowError);
}
