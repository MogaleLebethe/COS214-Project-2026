/**
 * @file test_strategy.cpp
 * @brief Strategy pattern: FieldValueRoutingStrategy, RouterRegistry and routing through DecisionTask.
 */
#include <gtest/gtest.h>

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

// ---------------------------------------------------------------- FieldValueRoutingStrategy

TEST(FieldValueRouting, ReturnsTheStringField) {
    FieldValueRoutingStrategy s("outcome");
    DecisionTask d;
    EXPECT_EQ("approve", s.route(d, {{"outcome", "approve"}}));
    EXPECT_EQ("outcome", s.getField());
}

TEST(FieldValueRouting, ConvertsNonStringValuesToText) {
    FieldValueRoutingStrategy s("approved");
    DecisionTask d;
    EXPECT_EQ("true", s.route(d, {{"approved", true}}));
    EXPECT_EQ("3", s.route(d, {{"approved", 3}}));
}

TEST(FieldValueRouting, MissingFieldThrowsNotFound) {
    FieldValueRoutingStrategy s("outcome");
    DecisionTask d;
    EXPECT_THROW(s.route(d, {{"other", "x"}}), NotFoundError);
    EXPECT_THROW(s.route(d, nlohmann::json()), NotFoundError);
    EXPECT_THROW(s.route(d, nlohmann::json::array()), NotFoundError);
}

// ---------------------------------------------------------------- RouterRegistry

TEST(RouterRegistryTest, AddGetHasKeys) {
    RouterRegistry routers;
    FieldValueRoutingStrategy* s = new FieldValueRoutingStrategy("outcome");
    routers.add("claimOutcome", s);

    EXPECT_EQ(s, routers.get("claimOutcome"));
    EXPECT_TRUE(routers.has("claimOutcome"));
    EXPECT_FALSE(routers.has("other"));
    EXPECT_EQ(std::vector<std::string>{"claimOutcome"}, routers.keys());
}

TEST(RouterRegistryTest, UnknownKeyThrowsNotFound) {
    RouterRegistry routers;
    EXPECT_THROW(routers.get("missing"), NotFoundError);
}

TEST(RouterRegistryTest, ReplacingDeletesOldAndDestructorDeletesAll) {
    liveStrategies = 0;
    {
        RouterRegistry routers;
        routers.add("a", new FixedRouter("1"));
        routers.add("b", new FixedRouter("2"));
        EXPECT_EQ(2, liveStrategies);

        routers.add("a", new FixedRouter("3"));
        EXPECT_EQ(2, liveStrategies);

        DecisionTask d;
        EXPECT_EQ("3", routers.get("a")->route(d, nlohmann::json()));
    }
    EXPECT_EQ(0, liveStrategies);
}

TEST(RouterRegistryTest, AddingSameStrategyUnderSameKeyIsHarmless) {
    RouterRegistry routers;
    FixedRouter* s = new FixedRouter("x");
    routers.add("a", s);
    routers.add("a", s);
    EXPECT_EQ(s, routers.get("a"));
}

TEST(RouterRegistryTest, MisuseThrowsLogicError) {
    RouterRegistry routers;
    FixedRouter* s = new FixedRouter("x");
    routers.add("a", s);

    EXPECT_THROW(routers.add("b", s), std::logic_error);
    EXPECT_THROW(routers.add("c", nullptr), std::logic_error);
    EXPECT_FALSE(routers.has("b"));
    EXPECT_FALSE(routers.has("c"));
}

// ---------------------------------------------------------------- Routing through DecisionTask

TEST(StrategyRouting, DecisionTaskUsesItsRouter) {
    RouterRegistry routers;
    routers.add("claimOutcome", new FieldValueRoutingStrategy("outcome"));
    DecisionTask d;
    d.addBranchTarget("approve", new Task());
    d.setRouter(routers.get("claimOutcome"));

    EXPECT_EQ("approve", d.decide({{"outcome", "approve"}}));
    EXPECT_EQ("approve", d.getSelectedBranch());
    for (Task* t : d.getBranchTargets()) delete t;
}

TEST(StrategyRouting, SwappingStrategyChangesTheDecision) {
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
    EXPECT_EQ("approve", d->decide(context));
    d->clearDecision();
    d->setRouter(&always);
    EXPECT_EQ("reject", d->decide(context));
}

TEST(StrategyRouting, IndustrySpecificStrategyRoutesOnData) {
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

    EXPECT_EQ("manager", d->decide({{"amount", 80000}}));
    d->clearDecision();
    EXPECT_EQ("auto", d->decide({{"amount", 1200}}));
}

TEST(StrategyRouting, DecisionWithoutRouterThrowsNotFound) {
    DecisionTask d;
    EXPECT_THROW(d.decide({{"outcome", "approve"}}), NotFoundError);
}

TEST(StrategyRouting, DecideCommandSkipsTheBranchTheRouterDidNotChoose) {
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
    EXPECT_EQ("Skipped", approve->getState()->toString());
    EXPECT_TRUE(reject->isReady());
}
