/**
 * @file test_strategy.cpp
 * @brief Strategy pattern (doctest): FieldValueRoutingStrategy, RouterRegistry and routing through DecisionTask.
 */
#include "doctest/doctest.h"

#include <stdexcept>
#include <string>
#include <vector>

#include "Command.h"
#include "DecisionTask.h"
#include "Project.h"
#include "RouterRegistry.h"
#include "RoutingStrategy.h"
#include "Support.h"
#include "Task.h"
#include "TaskState.h"
#include "WorkflowError.h"

namespace {

int liveStrategies = 0;

/** @brief Test strategy that always picks the same label and counts live instances. */
class FixedRouter : public RoutingStrategy {
public:
    explicit FixedRouter(const std::string& label) : label(label) { ++liveStrategies; }
    ~FixedRouter() override { --liveStrategies; }
    std::string route(const DecisionTask&, const nlohmann::json&) const override { return label; }

private:
    std::string label;
};

/** @brief Industry-style rule: claims above a limit need a manager. */
class AmountThresholdRouter : public RoutingStrategy {
public:
    explicit AmountThresholdRouter(double limit) : limit(limit) {}
    std::string route(const DecisionTask&, const nlohmann::json& context) const override {
        if (!context.contains("amount")) throw NotFoundError("amount missing");
        return context["amount"].get<double>() > limit ? "manager" : "auto";
    }

private:
    double limit;
};

}  // namespace

TEST_SUITE_BEGIN("strategy");

// ---------------------------------------------------------------- FieldValueRoutingStrategy

TEST_CASE("FieldValueRouting: returns the string field") {
    FieldValueRoutingStrategy s("outcome");
    DecisionTask d;
    CHECK(s.route(d, {{"outcome", "approve"}}) == "approve");
    CHECK(s.getField() == "outcome");
}

TEST_CASE("FieldValueRouting: converts non string values to text") {
    FieldValueRoutingStrategy s("approved");
    DecisionTask d;
    CHECK(s.route(d, {{"approved", true}}) == "true");
    CHECK(s.route(d, {{"approved", 3}}) == "3");
}

TEST_CASE("FieldValueRouting: missing field throws not found") {
    FieldValueRoutingStrategy s("outcome");
    DecisionTask d;
    CHECK_THROWS_AS(s.route(d, {{"other", "x"}}), NotFoundError);
    CHECK_THROWS_AS(s.route(d, nlohmann::json()), NotFoundError);
    CHECK_THROWS_AS(s.route(d, nlohmann::json::array()), NotFoundError);
}

// ---------------------------------------------------------------- RouterRegistry

TEST_CASE("RouterRegistryTest: add get has keys") {
    RouterRegistry routers;
    FieldValueRoutingStrategy* s = new FieldValueRoutingStrategy("outcome");
    routers.add("claimOutcome", s);

    CHECK(routers.get("claimOutcome") == s);
    CHECK(routers.has("claimOutcome"));
    CHECK_FALSE(routers.has("other"));
    CHECK(routers.keys() == std::vector<std::string>{"claimOutcome"});
}

TEST_CASE("RouterRegistryTest: unknown key throws not found") {
    RouterRegistry routers;
    CHECK_THROWS_AS(routers.get("missing"), NotFoundError);
}

TEST_CASE("RouterRegistryTest: replacing deletes old and destructor deletes all") {
    liveStrategies = 0;
    {
        RouterRegistry routers;
        routers.add("a", new FixedRouter("1"));
        routers.add("b", new FixedRouter("2"));
        CHECK(liveStrategies == 2);

        routers.add("a", new FixedRouter("3"));
        CHECK(liveStrategies == 2);

        DecisionTask d;
        CHECK(routers.get("a")->route(d, nlohmann::json()) == "3");
    }
    CHECK(liveStrategies == 0);
}

TEST_CASE("RouterRegistryTest: adding same strategy under same key is harmless") {
    RouterRegistry routers;
    FixedRouter* s = new FixedRouter("x");
    routers.add("a", s);
    routers.add("a", s);
    CHECK(routers.get("a") == s);
}

TEST_CASE("RouterRegistryTest: misuse throws logic error") {
    RouterRegistry routers;
    FixedRouter* s = new FixedRouter("x");
    routers.add("a", s);

    CHECK_THROWS_AS(routers.add("b", s), std::logic_error);
    CHECK_THROWS_AS(routers.add("c", nullptr), std::logic_error);
    CHECK_FALSE(routers.has("b"));
    CHECK_FALSE(routers.has("c"));
}

// ---------------------------------------------------------------- Routing through DecisionTask

TEST_CASE("StrategyRouting: decision task uses its router") {
    RouterRegistry routers;
    routers.add("claimOutcome", new FieldValueRoutingStrategy("outcome"));
    DecisionTask d;
    d.addBranchTarget("approve", new Task());
    d.setRouter(routers.get("claimOutcome"));

    CHECK(d.decide({{"outcome", "approve"}}) == "approve");
    CHECK(d.getSelectedBranch() == "approve");
    for (Task* t : d.getBranchTargets()) delete t;
}

TEST_CASE("StrategyRouting: swapping strategy changes the decision") {
    FixedRouter always("reject");
    FieldValueRoutingStrategy byField("outcome");
    Project project;
    DecisionTask* d = new DecisionTask();
    project.addWorkItem(d);
    Task* yes = new Task();
    Task* no = new Task();
    project.addWorkItem(yes);
    project.addWorkItem(no);
    d->addBranchTarget("approve", yes);
    d->addBranchTarget("reject", no);
    nlohmann::json context = {{"outcome", "approve"}};

    d->setRouter(&byField);
    CHECK(d->decide(context) == "approve");
    d->clearDecision();
    d->setRouter(&always);
    CHECK(d->decide(context) == "reject");
}

TEST_CASE("StrategyRouting: industry specific strategy routes on data") {
    AmountThresholdRouter rule(50000);
    Project project;
    DecisionTask* d = new DecisionTask();
    project.addWorkItem(d);
    Task* manager = new Task();
    Task* autoPay = new Task();
    project.addWorkItem(manager);
    project.addWorkItem(autoPay);
    d->addBranchTarget("manager", manager);
    d->addBranchTarget("auto", autoPay);
    d->setRouter(&rule);

    CHECK(d->decide({{"amount", 80000}}) == "manager");
    d->clearDecision();
    CHECK(d->decide({{"amount", 1200}}) == "auto");
}

TEST_CASE("StrategyRouting: decision without router throws not found") {
    DecisionTask d;
    CHECK_THROWS_AS(d.decide({{"outcome", "approve"}}), NotFoundError);
}

TEST_CASE("StrategyRouting: decide command skips the branch the router did not choose") {
    RouterRegistry routers;
    routers.add("claimOutcome", new FieldValueRoutingStrategy("outcome"));
    Project project;
    CommandHistory history;
    DecisionTask* d = new DecisionTask();
    Task* approve = new Task();
    Task* reject = new Task();
    project.addWorkItem(d);
    project.addWorkItem(approve);
    project.addWorkItem(reject);
    d->setRouter(routers.get("claimOutcome"));
    d->addBranchTarget("approve", approve);
    d->addBranchTarget("reject", reject);
    d->start();

    history.executeCommand(new DecideCommand(d, {{"outcome", "reject"}}));
    CHECK(approve->getState()->toString() == "Skipped");
    CHECK(reject->isReady());
}

TEST_SUITE_END();
