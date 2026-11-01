#include <cassert>
#include <iostream>
#include "Task.h"
#include "TaskState.h"
#include "WorkflowError.h"

int main(){
    Task t1; 
    assert(t1.getState()->toString() == "Pending");
    t1.start();
    assert(t1.getState()->toString() == "In Progress");
    t1.complete(); 
    assert(t1.getState()->toString() =="Completed"); 
    bool threw = false; 
    try{
        t1.complete();

    } catch (const WorkflowError&){
        threw = true;
    }
    assert(threw);

    Task t2;
    t2.block();
    assert(t2.getState()->toString() == "Blocked");
    t2.start();
    assert(t2.getState()->toString() == "In Progress");
    std::cout << "All tests pass" << std::endl; 
    return 0; 


}