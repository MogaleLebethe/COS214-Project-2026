#include <cassert>
#include <iostream>
#include "Task.h"
#include "TaskState.h"
#include "TaskStateFactory.h"
#include "WorkflowError.h"

int main(){
    TaskState* p1 = TaskStateFactory::get("Pending"); 
    TaskState* p2 = TaskStateFactory::get("Pending");
    assert(p1 == p2); 
    Task t1; 
    Task t2; 
    assert(t1.getState() == t2.getState()); 
    assert(t2.getState()->toString() == "Pending");

    bool threwNotF = false;
    try{
        TaskStateFactory::get("NotARealState");
    }
    catch (const WorkflowError&){
        threwNotF = true; 
    }
    assert(threwNotF);
    std::cout << "All test pass" << std::endl;
    return 0; 
}