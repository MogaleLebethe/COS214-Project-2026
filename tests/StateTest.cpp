#include "doctest/doctest.h"
#include "Task.h"
#include "TaskState.h"
#include "WorkflowError.h"

TEST_CASE("a new task starts in Pending"){
    Task t; 
    CHECK(t.getState()->toString() == "Pending");
}

TEST_CASE("start() trans Pending to In Progress"){
    Task t; 
    t.start();
    CHECK(t.getState()->toString() == "In Progress");
}

TEST_CASE("complete() transition In Progress to Completed"){
    Task t; 
    t.start(); 
    t.complete(); 
    CHECK(task.getState()->toString() == "Completed");
}

TEST_CASE("completing an already-completed task is invalid"){
    Task t; 
    t.start(); 
    t.complete(); 
    CHECK THROWS AS(t.complete(), WorkflowError);
}
TEST_CASE("a Pending task can be blocked, then starte again"){
    Task t; 
    t.block(); 
    CHECK(t.getState()->toString() == "Blocked");
    t.start(); 
    CHECK(t.getState()->toString() == "In Progress");
}