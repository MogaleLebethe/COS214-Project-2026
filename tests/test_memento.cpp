/**
 * @file test_memento.cpp
 * @brief Memento pattern: TaskMemento, Task, EditTaskInfoCommand
 */
#include "doctest/doctest.h"

#include "Task.h"
#include "TaskMemento.h"
#include "Support.h"
#include "WorkflowError.h"


TEST_CASE("Default: TaskMemento is empty"){
    TaskMemento memento;

    CHECK(memento.isEmpty());
}


TEST_CASE("Task can create a memento containing its state"){
    Task task;

    task.setName("Test Task");
    task.setDescription("Test Description");
    task.setExecutor(Participant("P1", "Test Participant", "Developer"));
    task.setDeadline(DateTime(2026, 12, 25));
    task.setCustomField("priority", "high");

    TaskMemento* memento = task.createMemento();

    REQUIRE(memento != nullptr);
    CHECK_FALSE(memento->isEmpty());

    delete memento;
}


TEST_CASE("Task restores its state from a memento") {
    Task task;

    task.setName("Original Name");
    task.setDescription("Original Description");
    task.setExecutor(Participant("P1", "Test Participant", "Developer"));
    task.setDeadline(DateTime(2026, 12, 25));
    task.setCustomField("priority", "high");

    TaskMemento* memento = task.createMemento();
    REQUIRE(memento != nullptr);

    task.setName("Changed Name");
    task.setDescription("Changed Description");
    task.setExecutor(Participant("P2", "Other Participant", "Manager"));
    task.setDeadline(DateTime(2027, 1, 1));
    task.setCustomField("priority", "low");

    task.restoreMemento(memento);

    // CHECK: state is actually restored
    nlohmann::json restored = task.toJson();

    CHECK(restored["name"] == "Original Name");
    CHECK(restored["description"] == "Original Description");
    CHECK(restored["executor"]["id"] == "P1");
    CHECK(restored["customFields"]["priority"] == "high");
    CHECK(restored["deadline"] == "2026-12-25 00:00");

    delete memento;
}


TEST_CASE("Restoring a null memento throws an exception") {
    Task task;

    CHECK_THROWS_AS(task.restoreMemento(nullptr),WorkflowError);
}
