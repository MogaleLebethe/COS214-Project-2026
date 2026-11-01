/**
 * @file test_builder.cpp
 * @brief Builder pattern: TaskBuilder, ProjectBuilder, DecisionTaskBuilder, Director and the WorkflowFactory facade.
 */
#include "doctest/doctest.h"

#include <stdexcept>
#include <string>

#include "Support.h"   // must precede WorkflowComponent.h, which uses DateTime without including it
#include "DecisionTask.h"
#include "Project.h"
#include "RouterRegistry.h"
#include "RoutingStrategy.h"
#include "Task.h"
#include "WorkflowComponent.h"
#include "WorkflowError.h"
#include "WorkflowFactory.h"

TEST_CASE("TaskBuilder assigns every field it is given") {
    TaskBuilder builder;
    DateTime deadline(2026, 3, 15, 9, 30);
    builder.setBasicInfo("T-custom", "Draft the brief");
    builder.setDescription("First pass of the client brief");
    builder.setExecutor(Participant("P-1", "Alex", "Copywriter"));
    builder.setDeadline(deadline);
    builder.addCustomField("priority", "high");

    Task* task = builder.getResult();
    REQUIRE(task != nullptr);
    CHECK(task->getId() == "T-custom");
    CHECK(task->getName() == "Draft the brief");
    CHECK(task->getDescription() == "First pass of the client brief");
    CHECK(task->getExecutorId() == "P-1");
    CHECK(task->getCustomField("priority") == "high");

    nlohmann::json j = task->toJson();
    CHECK(j["executor"]["name"] == "Alex");
    CHECK(j["executor"]["role"] == "Copywriter");
    CHECK(j["deadline"] == deadline.toString("%Y-%m-%d %H:%M"));
    delete task;
}

TEST_CASE("TaskBuilder keeps the generated id when none is supplied") {
    TaskBuilder builder;
    builder.setBasicInfo("", "Generated id");

    Task* task = builder.getResult();
    REQUIRE(task != nullptr);
    CHECK_FALSE(task->getId().empty());
    delete task;
}

TEST_CASE("build() rejects a component with no name") {
    SUBCASE("TaskBuilder") {
        TaskBuilder builder;
        CHECK_THROWS_AS(builder.build(), WorkflowError);
        CHECK_THROWS_AS(builder.getResult(), WorkflowError);
    }
    SUBCASE("ProjectBuilder") {
        ProjectBuilder builder;
        CHECK_THROWS_AS(builder.build(), WorkflowError);
        CHECK_THROWS_AS(builder.getResult(), WorkflowError);
    }
    SUBCASE("DecisionTaskBuilder") {
        DecisionTaskBuilder builder;
        CHECK_THROWS_AS(builder.build(), WorkflowError);
        CHECK_THROWS_AS(builder.getDecisionResult(), WorkflowError);
    }
}

TEST_CASE("a rejected build() leaves the builder usable") {
    TaskBuilder builder;
    CHECK_THROWS_AS(builder.build(), WorkflowError);

    builder.setBasicInfo("", "Named after the failure");
    Task* task = builder.getResult();
    REQUIRE(task != nullptr);
    CHECK(task->getName() == "Named after the failure");
    delete task;
}

TEST_CASE("TaskBuilder refuses nested work items") {
    TaskBuilder builder;
    builder.setBasicInfo("", "Leaf");

    TaskBuilder childBuilder;
    childBuilder.setBasicInfo("", "Child");
    Task* child = childBuilder.getResult();

    CHECK_THROWS_AS(builder.addWorkItem(child), std::logic_error);
    delete child;
}

TEST_CASE("ProjectBuilder assigns fields and attaches nested work items") {
    TaskBuilder taskBuilder;
    taskBuilder.setBasicInfo("T-mock", "Design the mockups");
    Task* mockups = taskBuilder.getResult();

    taskBuilder.setBasicInfo("T-copy", "Write the copy");
    Task* copy = taskBuilder.getResult();

    ProjectBuilder builder;
    builder.setBasicInfo("P-rebrand", "Q1 Rebrand Campaign");
    builder.setDescription("Full rebrand for the client");
    builder.addCustomField("client", "Acme");
    builder.addWorkItem(mockups);
    builder.addWorkItem(copy);

    Project* project = builder.getResult();
    REQUIRE(project != nullptr);
    CHECK(project->getId() == "P-rebrand");
    CHECK(project->getName() == "Q1 Rebrand Campaign");
    CHECK(project->getDescription() == "Full rebrand for the client");
    CHECK(project->getCustomField("client") == "Acme");

    REQUIRE(project->getWorkItems().size() == 2);
    CHECK(project->getWorkItems()[0] == mockups);
    CHECK(project->getWorkItems()[1] == copy);
    CHECK(project->findById("T-copy") == copy);
    delete project;   // the project owns its work items
}

TEST_CASE("ProjectBuilder can nest a project inside a project") {
    ProjectBuilder innerBuilder;
    innerBuilder.setBasicInfo("", "Phase 1");
    Project* inner = innerBuilder.getResult();

    ProjectBuilder outerBuilder;
    outerBuilder.setBasicInfo("", "Programme");
    outerBuilder.addWorkItem(inner);
    Project* outer = outerBuilder.getResult();

    REQUIRE(outer->getWorkItems().size() == 1);
    CHECK(outer->getWorkItems()[0] == inner);
    CHECK(outer->findById(inner->getId()) == inner);
    delete outer;
}

TEST_CASE("repeated getResult() calls hand back isolated components") {
    TaskBuilder builder;
    builder.setBasicInfo("", "First task");
    builder.setDescription("only for the first");
    builder.addCustomField("tag", "one");
    Task* first = builder.getResult();

    builder.setBasicInfo("", "Second task");
    Task* second = builder.getResult();

    CHECK(first != second);
    CHECK(first->getId() != second->getId());
    CHECK(first->getName() == "First task");
    CHECK(first->getDescription() == "only for the first");
    CHECK(first->getCustomField("tag") == "one");
    CHECK(second->getName() == "Second task");
    CHECK(second->getDescription().empty());
    CHECK(second->getCustomField("tag").empty());

    delete first;
    delete second;
}

TEST_CASE("a finished product is not carried into the next build") {
    TaskBuilder builder;
    builder.setBasicInfo("", "One-off");
    Task* done = builder.getResult();
    delete done;

    // The builder was reset after handing the product over, so the name is gone.
    CHECK_THROWS_AS(builder.getResult(), WorkflowError);
}

TEST_CASE("reset() discards a half-built component") {
    TaskBuilder builder;
    builder.setBasicInfo("", "Abandoned");
    builder.reset();

    CHECK_THROWS_AS(builder.getResult(), WorkflowError);
}

TEST_CASE("DecisionTaskBuilder wires a routing strategy to its branches") {
    TaskBuilder branchBuilder;
    branchBuilder.setBasicInfo("", "Send approval email");
    Task* approveBranch = branchBuilder.getResult();
    branchBuilder.setBasicInfo("", "Send rejection email");
    Task* rejectBranch = branchBuilder.getResult();

    FieldValueRoutingStrategy router("outcome");
    DecisionTaskBuilder builder;
    builder.setBasicInfo("", "Review client response");
    builder.setRouter(&router);
    builder.addBranchTarget("approve", approveBranch);
    builder.addBranchTarget("reject", rejectBranch);

    DecisionTask* decision = builder.getDecisionResult();
    REQUIRE(decision != nullptr);
    CHECK(decision->getName() == "Review client response");
    CHECK(decision->getBranches().size() == 2);
    CHECK(decision->getBranchTargets().size() == 2);

    CHECK(decision->decide(nlohmann::json{{"outcome", "approve"}}) == "approve");
    CHECK(decision->getSelectedBranch() == "approve");
    CHECK(decision->decide(nlohmann::json{{"outcome", "reject"}}) == "reject");
    CHECK(decision->getSelectedBranch() == "reject");

    CHECK_THROWS_AS(decision->decide(nlohmann::json{{"outcome", "escalate"}}), NotFoundError);
    CHECK_THROWS_AS(decision->decide(nlohmann::json{{"unrelated", "approve"}}), NotFoundError);

    delete decision;
    delete approveBranch;
    delete rejectBranch;
}

TEST_CASE("DecisionTaskBuilder::build() returns a DecisionTask") {
    DecisionTaskBuilder builder;
    builder.setBasicInfo("", "Fork");

    WorkflowComponent* component = builder.build();
    REQUIRE(component != nullptr);
    CHECK(dynamic_cast<DecisionTask*>(component) != nullptr);
    delete component;
}

TEST_CASE("DecisionTaskBuilder rejects bad branch targets") {
    DecisionTaskBuilder builder;
    builder.setBasicInfo("", "Fork");
    CHECK_THROWS_AS(builder.addBranchTarget("approve", nullptr), WorkflowError);

    TaskBuilder branchBuilder;
    branchBuilder.setBasicInfo("", "Branch");
    Task* branch = branchBuilder.getResult();
    CHECK_THROWS_AS(builder.addBranchTarget("", branch), WorkflowError);

    DecisionTask* decision = builder.getDecisionResult();
    delete decision;
    delete branch;
}

TEST_CASE("Director builds named components through its registered builders") {
    TaskBuilder taskBuilder;
    ProjectBuilder projectBuilder;
    Director director;
    director.registerBuilder("Task", &taskBuilder);
    director.registerBuilder("Project", &projectBuilder);

    Task* task = director.createNewTask("Book the venue", "Hall for 80 guests");
    REQUIRE(task != nullptr);
    CHECK(task->getName() == "Book the venue");
    CHECK(task->getDescription() == "Hall for 80 guests");
    delete task;

    Project* project = director.createNewProject("Launch party", "Autumn launch");
    REQUIRE(project != nullptr);
    CHECK(project->getName() == "Launch party");
    CHECK(project->getDescription() == "Autumn launch");
    delete project;
}

TEST_CASE("Director reports missing or invalid builders") {
    Director director;
    CHECK_THROWS_AS(director.createNewTask("No builder"), WorkflowError);
    CHECK_THROWS_AS(director.createNewProject("No builder"), WorkflowError);
    CHECK_THROWS_AS(director.registerBuilder("Task", nullptr), std::invalid_argument);

    nlohmann::json untyped = nlohmann::json{{"name", "Untyped"}};
    CHECK_THROWS_AS(director.createFromTemplate(untyped), WorkflowError);

    nlohmann::json unknown = nlohmann::json{{"type", "Spaceship"}, {"name", "Unknown"}};
    CHECK_THROWS_AS(director.createFromTemplate(unknown), NotFoundError);
}

TEST_CASE("Director hydrates a nested tree from a JSON template") {
    TaskBuilder taskBuilder;
    ProjectBuilder projectBuilder;
    Director director;
    director.registerBuilder("Task", &taskBuilder);
    director.registerBuilder("Project", &projectBuilder);

    nlohmann::json tmpl = {
        {"type", "Project"},
        {"name", "Onboarding"},
        {"description", "Client onboarding"},
        {"customFields", {{"tier", "gold"}}},
        {"workItems", {
            {{"type", "Task"}, {"name", "Kickoff call"},
             {"executor", {{"id", "P-7"}, {"name", "Sam"}, {"role", "Account lead"}}}},
            {{"type", "Task"}, {"name", "Send contract"}}
        }}
    };

    WorkflowComponent* built = director.createFromTemplate(tmpl);
    Project* project = dynamic_cast<Project*>(built);
    REQUIRE(project != nullptr);
    CHECK(project->getName() == "Onboarding");
    CHECK(project->getCustomField("tier") == "gold");
    REQUIRE(project->getWorkItems().size() == 2);
    CHECK(project->getWorkItems()[0]->getName() == "Kickoff call");
    CHECK(project->getWorkItems()[0]->getExecutorId() == "P-7");
    CHECK(project->getWorkItems()[1]->getName() == "Send contract");
    delete project;
}

TEST_CASE("WorkflowFactory facade produces named components") {
    WorkflowFactory factory;

    Task* task = factory.newTask("Prepare invoice", "Q4 invoice run");
    REQUIRE(task != nullptr);
    CHECK(task->getName() == "Prepare invoice");
    CHECK(task->getDescription() == "Q4 invoice run");
    delete task;

    Project* project = factory.newProject("Client Onboarding", "New accounts");
    REQUIRE(project != nullptr);
    CHECK(project->getName() == "Client Onboarding");
    CHECK(project->getDescription() == "New accounts");
    delete project;
}

TEST_CASE("WorkflowFactory hands out independent components on every call") {
    WorkflowFactory factory;

    Task* a = factory.newTask("Same name");
    Task* b = factory.newTask("Same name");
    CHECK(a != b);
    CHECK(a->getId() != b->getId());
    delete a;
    delete b;
}

TEST_CASE("WorkflowFactory rejects an empty name") {
    WorkflowFactory factory;
    CHECK_THROWS_AS(factory.newTask(""), WorkflowError);
    CHECK_THROWS_AS(factory.newProject(""), WorkflowError);
}

TEST_CASE("WorkflowFactory builds a routed decision task from a template") {
    RouterRegistry routers;
    routers.add("byOutcome", new FieldValueRoutingStrategy("outcome"));
    WorkflowFactory factory(&routers);

    nlohmann::json tmpl = {
        {"type", "Decision"},
        {"name", "Approve the quote"},
        {"router", "byOutcome"}
    };

    WorkflowComponent* built = factory.fromTemplate(tmpl);
    DecisionTask* decision = dynamic_cast<DecisionTask*>(built);
    REQUIRE(decision != nullptr);
    CHECK(decision->getName() == "Approve the quote");

    TaskBuilder branchBuilder;
    branchBuilder.setBasicInfo("", "Issue purchase order");
    Task* branch = branchBuilder.getResult();
    decision->addBranchTarget("yes", branch);

    CHECK(decision->decide(nlohmann::json{{"outcome", "yes"}}) == "yes");

    delete decision;
    delete branch;
}
