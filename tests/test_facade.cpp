/**
 * @file test_facade.cpp
 * @brief Facade pattern (doctest): the whole engine driven only through WorkflowEngineFacade.
 */
#include "doctest/doctest.h"

#include <string>
#include <vector>

#include "ActionRegistry.h"
#include "DefinitionRepository.h"
#include "ProjectRegistry.h"
#include "ReportService.h"
#include "RouterRegistry.h"
#include "RoutingStrategy.h"
#include "Support.h"
#include "WorkflowEngineFacade.h"
#include "WorkflowError.h"
#include "WorkflowFactory.h"

namespace {

/** @brief Owns every subsystem and wires them into one Facade, like main() does. */
struct FacadeFixture {
    RouterRegistry routers;
    ProjectRegistry registry;
    WorkflowFactory factory{&routers};
    ReportService reports;
    DefinitionRepository definitions;
    ActionRegistry actions;
    WorkflowEngineFacade facade{registry, factory, reports, definitions, actions, routers};

    /** @brief Status of a work item, read from the exported JSON (as the UI would). */
    std::string status(const std::string& projectId, const std::string& itemId) {
        nlohmann::json p = facade.exportProjectToJson(projectId);
        for (const auto& item : p["workItems"])
            if (item["id"] == itemId) return item["status"].get<std::string>();
        return "<missing>";
    }

    std::string name(const std::string& projectId, const std::string& itemId) {
        nlohmann::json p = facade.exportProjectToJson(projectId);
        for (const auto& item : p["workItems"])
            if (item["id"] == itemId) return item["name"].get<std::string>();
        return "<missing>";
    }

    bool isAvailable(const std::string& projectId, const std::string& itemId) {
        for (const WorkItemView& v : facade.listAvailableWork(projectId))
            if (v.id == itemId) return true;
        return false;
    }

    std::string firstWorkItemId(const std::string& projectId) {
        return facade.exportProjectToJson(projectId)["workItems"][0]["id"].get<std::string>();
    }
};

nlohmann::json claimTemplate() {
    return {{"type", "Project"},
            {"name", "Claim template"},
            {"workItems", {{{"type", "Task"}, {"name", "Assess claim"}},
                           {{"type", "Task"}, {"name", "Pay out"}}}}};
}

}  // namespace

TEST_SUITE_BEGIN("facade");

// ---------------------------------------------------------------- projects

TEST_CASE_FIXTURE(FacadeFixture, "created projects are listed with unique ids") {
    std::string a = facade.createProject("Admissions");
    std::string b = facade.createProject("Claims");
    std::vector<std::string> ids = facade.listProjects();
    CHECK(ids.size() == 2);
    CHECK(a != b);
}

TEST_CASE_FIXTURE(FacadeFixture, "unknown project id is a NotFoundError") {
    CHECK_THROWS_AS(facade.addTaskToProject("no-such-project", "Task"), NotFoundError);
    CHECK_THROWS_AS(facade.viewProjectStatus("no-such-project"), NotFoundError);
}

TEST_CASE_FIXTURE(FacadeFixture, "added task is pending and available") {
    std::string p = facade.createProject("Claims");
    std::string t = facade.addTaskToProject(p, "Assess claim");
    CHECK(status(p, t) == "Pending");
    CHECK(isAvailable(p, t));
    CHECK(facade.viewProjectStatus(p).find("Assess claim") != std::string::npos);
}

// ---------------------------------------------------------------- actions (Command + State through the Facade)

TEST_CASE_FIXTURE(FacadeFixture, "start and complete through performAction") {
    std::string p = facade.createProject("Claims");
    std::string t = facade.addTaskToProject(p, "Assess claim");
    facade.performAction(p, t, "start");
    CHECK(status(p, t) == "In Progress");
    facade.performAction(p, t, "complete");
    CHECK(status(p, t) == "Complete");
    CHECK_FALSE(isAvailable(p, t));
}

TEST_CASE_FIXTURE(FacadeFixture, "illegal action is rejected and changes nothing") {
    std::string p = facade.createProject("Claims");
    std::string t = facade.addTaskToProject(p, "Assess claim");
    CHECK_THROWS_AS(facade.performAction(p, t, "complete"), InvalidTransitionError);
    CHECK(status(p, t) == "Pending");
}

TEST_CASE_FIXTURE(FacadeFixture, "unknown action is a NotFoundError") {
    std::string p = facade.createProject("Claims");
    std::string t = facade.addTaskToProject(p, "Assess claim");
    CHECK_THROWS_AS(facade.performAction(p, t, "teleport"), NotFoundError);
}

TEST_CASE_FIXTURE(FacadeFixture, "exception path: fail then retry") {
    std::string p = facade.createProject("Payments");
    std::string t = facade.addTaskToProject(p, "Charge card");
    facade.performAction(p, t, "start");
    facade.performAction(p, t, "fail");
    CHECK(status(p, t) == "Failed");
    facade.performAction(p, t, "start");
    CHECK(status(p, t) == "In Progress");
}

// ---------------------------------------------------------------- dependencies (Observer through the Facade)

TEST_CASE_FIXTURE(FacadeFixture, "dependent task becomes available only after its prerequisite completes") {
    std::string p = facade.createProject("Claims");
    std::string assess = facade.addTaskToProject(p, "Assess claim");
    std::string pay = facade.addTaskToProject(p, "Pay out");
    facade.addDependency(p, pay, assess);

    CHECK_FALSE(isAvailable(p, pay));
    CHECK_THROWS_AS(facade.performAction(p, pay, "start"), InvalidTransitionError);

    facade.performAction(p, assess, "start");
    facade.performAction(p, assess, "complete");
    CHECK(isAvailable(p, pay));
}

TEST_CASE_FIXTURE(FacadeFixture, "cyclic dependency is rejected") {
    std::string p = facade.createProject("Claims");
    std::string a = facade.addTaskToProject(p, "A");
    std::string b = facade.addTaskToProject(p, "B");
    facade.addDependency(p, b, a);
    CHECK_THROWS_AS(facade.addDependency(p, a, b), CyclicDependencyError);
}

// ---------------------------------------------------------------- undo / redo

TEST_CASE_FIXTURE(FacadeFixture, "undo and redo through the facade") {
    std::string p = facade.createProject("Claims");
    std::string t = facade.addTaskToProject(p, "Assess claim");
    facade.performAction(p, t, "start");
    CHECK(facade.canUndo(p));
    CHECK_FALSE(facade.canRedo(p));

    facade.undo(p);
    CHECK(status(p, t) == "Pending");
    CHECK(facade.canRedo(p));

    facade.redo(p);
    CHECK(status(p, t) == "In Progress");
}

TEST_CASE_FIXTURE(FacadeFixture, "undo with nothing to undo is a WorkflowError") {
    std::string p = facade.createProject("Claims");
    CHECK_THROWS_AS(facade.undo(p), WorkflowError);
}

TEST_CASE_FIXTURE(FacadeFixture, "undo of add task removes it") {
    std::string p = facade.createProject("Claims");
    std::string t = facade.addTaskToProject(p, "Temporary");
    facade.undo(p);
    CHECK(status(p, t) == "<missing>");
}

// ---------------------------------------------------------------- editing (Memento through the Facade)

TEST_CASE_FIXTURE(FacadeFixture, "editTask changes fields and undo restores them") {
    std::string p = facade.createProject("Claims");
    std::string t = facade.addTaskToProject(p, "Assess", "first look");
    facade.editTask(p, t, {{"name", "Assess claim"}, {"customFields", {{"priority", "high"}}}});
    CHECK(name(p, t) == "Assess claim");

    facade.undo(p);
    CHECK(name(p, t) == "Assess");
}

TEST_CASE_FIXTURE(FacadeFixture, "editTask with an unknown field is rejected") {
    std::string p = facade.createProject("Claims");
    std::string t = facade.addTaskToProject(p, "Assess");
    CHECK_THROWS_AS(facade.editTask(p, t, {{"colour", "blue"}}), WorkflowError);
    CHECK(name(p, t) == "Assess");
}

TEST_CASE_FIXTURE(FacadeFixture, "assignExecutor shows the executor on available work") {
    std::string p = facade.createProject("Claims");
    std::string t = facade.addTaskToProject(p, "Assess claim");
    facade.assignExecutor(p, t, Participant("u7", "Thandi", "Assessor"));

    std::string executor;
    for (const WorkItemView& v : facade.listAvailableWork(p))
        if (v.id == t) executor = v.executor;
    CHECK(executor == "u7");
}

// ---------------------------------------------------------------- history (audit log)

TEST_CASE_FIXTURE(FacadeFixture, "history records executed, rejected and undone actions") {
    std::string p = facade.createProject("Claims");
    std::string t = facade.addTaskToProject(p, "Assess claim");
    std::size_t before = facade.getHistory(p).size();

    facade.performAction(p, t, "start");
    CHECK_THROWS(facade.performAction(p, t, "start"));
    facade.undo(p);

    CHECK(facade.getHistory(p).size() == before + 3);
}

// ---------------------------------------------------------------- routing (Strategy through the Facade)

TEST_CASE_FIXTURE(FacadeFixture, "decide on a normal task is rejected") {
    std::string p = facade.createProject("Claims");
    std::string t = facade.addTaskToProject(p, "Assess claim");
    CHECK_THROWS_AS(facade.decide(p, t, {{"outcome", "approve"}}), WorkflowError);
}

// ---------------------------------------------------------------- templates, definitions and instances (Builder + Prototype)

TEST_CASE_FIXTURE(FacadeFixture, "createFromTemplate builds the work items") {
    std::string p = facade.createFromTemplate(claimTemplate());
    CHECK(facade.exportProjectToJson(p)["workItems"].size() == 2);
    CHECK(facade.listAvailableWork(p).size() == 2);
}

TEST_CASE_FIXTURE(FacadeFixture, "template whose root is not a Project is rejected") {
    nlohmann::json notAProject = {{"type", "Task"}, {"name", "Lonely task"}};
    CHECK_THROWS_AS(facade.createFromTemplate(notAProject), WorkflowError);
    CHECK(facade.listProjects().empty());
}

TEST_CASE_FIXTURE(FacadeFixture, "instances started from one definition are independent") {
    definitions.add(WorkflowDefinition("claim-v1", "Claim", "Insurance", "Claim assessment", claimTemplate()));
    std::string first = facade.startInstance("claim-v1", "Claim #1");
    std::string second = facade.startInstance("claim-v1", "Claim #2");
    CHECK(first != second);

    std::string task = firstWorkItemId(first);
    facade.performAction(first, task, "start");

    CHECK(status(first, task) == "In Progress");
    CHECK(status(second, firstWorkItemId(second)) == "Pending");
}

TEST_CASE_FIXTURE(FacadeFixture, "unknown definition is a NotFoundError") {
    CHECK_THROWS_AS(facade.startInstance("no-such-definition", "X"), NotFoundError);
}

TEST_CASE_FIXTURE(FacadeFixture, "cloned project runs independently of the original") {
    std::string original = facade.createProject("Claims");
    std::string task = facade.addTaskToProject(original, "Assess claim");
    std::string copy = facade.cloneProject(original, "Claims copy");
    CHECK(copy != original);

    std::string copiedTask = firstWorkItemId(copy);
    facade.performAction(copy, copiedTask, "start");

    CHECK(status(copy, copiedTask) == "In Progress");
    CHECK(status(original, task) == "Pending");
}

TEST_SUITE_END();
