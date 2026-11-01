#include "doctest/doctest.h"
#include "WorkflowFactory.h"
#include "Task.h"
#include "Project.h"
#include "DecisionTask.h"
#include "WorkflowComponent.h"
#include "Support.h"
#include "WorkflowError.h"
#include "RouterRegistry.h"
#include "RoutingStrategy.h"

TEST_CASE("TaskBuilder produces a task with the right fields") {
    TaskBuilder builder;
    builder.setBasicInfo("","Draft the brief");
    builder.setExecutor(Participant("P-1", "Alex", "Copywriter"));

    Task* task=builder.getResult();
    CHECK(task!=nullptr);
    CHECK(task->getName()=="Draft the brief");
    CHECK(task->getExecutorId() =="P-1");
    delete task;
}

TEST_CASE("TaskBuilder rejects build() with no name") {
    TaskBuilder builder;

    CHECK_THROWS_AS(builder.build(),WorkflowError);
}

TEST_CASE("ProjectBuilder attaches nested work items") {
    TaskBuilder childBuilder;
    childBuilder.setBasicInfo("","Design the mockups");
    Task* child = childBuilder.getResult();

    ProjectBuilder builder;
    builder.setBasicInfo("","Q1 Rebrand Campaign");
    builder.addWorkItem(child);

    Project* project = builder.getResult();
    CHECK(project!=nullptr);
    CHECK(project->getWorkItems().size()== 1);

    delete project;
}

TEST_CASE("builder instances stay isolated across repeated builds") {
    TaskBuilder builder;
    builder.setBasicInfo("","First task");
    Task* first = builder.getResult();

    builder.setBasicInfo("", "Second task");
    Task* second = builder.getResult();

    CHECK(first != second);
    CHECK(first->getName()=="First task");
    CHECK(second->getName()== "Second task");

    delete first;
    delete second;
}

TEST_CASE("DecisionTaskBuilder routes to the correct branch") {
    TaskBuilder approveBuilder;
    approveBuilder.setBasicInfo("","Send approval email");
    Task* approveBranch = approveBuilder.getResult();

    TaskBuilder rejectBuilder;
    rejectBuilder.setBasicInfo("","Send rejection email");
    Task* rejectBranch = rejectBuilder.getResult();

    FieldValueRoutingStrategy router("outcome");
    DecisionTaskBuilder builder;
    builder.setBasicInfo("","Review client response");
    builder.setRouter(&router);
    builder.addBranchTarget("approve",approveBranch);
    builder.addBranchTarget("reject",rejectBranch);

    DecisionTask* decision = builder.getDecisionResult();
    REQUIRE(decision != nullptr);
    CHECK(decision->getBranches().size() ==2);
    CHECK(decision->decide(nlohmann::json{{"outcome", "approve"}})== "approve");

    delete decision;
    delete approveBranch;
    delete rejectBranch;
}

TEST_CASE("Director drives builders through a registered recipe") {
    TaskBuilder taskBuilder;
    ProjectBuilder projectBuilder;
    Director director;
    director.registerBuilder("Task",&taskBuilder);
    director.registerBuilder("Project", &projectBuilder);

    Task* task =director.createNewTask("Book the venue");
    CHECK(task !=nullptr);
    delete task;
}

TEST_CASE("WorkflowFactory facade produces named components") {
    WorkflowFactory factory;

    Task* task=factory.newTask("Prepare invoice");
    CHECK(task!=nullptr);
    delete task;

    Project* project= factory.newProject("Client Onboarding");
    CHECK(project!= nullptr);
    
    delete project;
}
