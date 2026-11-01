#include "doctest/doctest.h"
#include "Task.h"
#include "TaskState.h"
#include "TaskStateFactory.h"
#include "WorkflowError.h"

TEST_CASE("factory returns the same singleton for the same name"){
    TaskState* p1 = TaskStateFactory::get("Pending"); 
    TaskState* p2 = TaskStateFactory::get("Pending");
    CHECK(p1 == p2); 
}

TEST_CASE("two tasks in same state share one state instance"){
    Task t1; 
    Task t2; 
    CHECK(t1.getState() == t2.getState()); 
}

TEST_CASE("moving one tasks state dont impact another instace"){
    Task t1; 
    Task t2; 
    t1.start(); 
    CHECK(t1.getState() != t2.getState());
    CHECK(t2.getState()->toString() == "Pending"); 

}

TEST_CASE(" unknow state name is rejected"){
    CHECK_THROWS_AS(TaskStateFactory::get("NotARealState"), WorkflowError); 
}