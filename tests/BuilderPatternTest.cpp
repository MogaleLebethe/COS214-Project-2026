#include <cassert>
#include <iostream>
#include "WorkflowFactory.h"
#include "Task.h"
#include "Project.h"
#include "DecisionTask.h"
#include "WorkflowComponent.h"
#include "Support.h"
#include "WorkflowError.h"
#include "RouterRegistry.h"
#include "RoutingStrategy.h"

int main() {

    TaskBuilder taskBuilder;
    taskBuilder.setBasicInfo("","Draft brief");
    taskBuilder.setExecutor(Participant("P-1" ,"Christiano","Copywriter"));
    
    Task* task = taskBuilder.getResult();
    assert(task != nullptr);
    assert(task->getName() == "Draft brief");
    assert(task->getExecutorId()=="P-1");
    delete task;

    TaskBuilder childBuilder;

    childBuilder.setBasicInfo("","Design mocks");
    Task*child =childBuilder.getResult() ;

    ProjectBuilder projectBuilder;
    projectBuilder.setBasicInfo("", "UCL season");
    projectBuilder.addWorkItem(child);
    Project* project=projectBuilder.getResult();

    assert(project !=nullptr);
    assert(project->getWorkItems().size()== 1);
    delete project;

    TaskBuilder reuseBuilder;
    reuseBuilder.setBasicInfo("","T1");

    Task* first = reuseBuilder.getResult();
    reuseBuilder.setBasicInfo("","T2");
    Task* second = reuseBuilder.getResult();
    assert(first !=second);

    assert(first->getName()=="T1");
    assert(second->getName()== "T2");

    delete first;
    delete second;

    TaskBuilder approveBuilder;
    approveBuilder.setBasicInfo("","Sub defenders");
    Task* approveBranch = approveBuilder.getResult();

    TaskBuilder rejectBuilder;
    rejectBuilder.setBasicInfo("", "Grill attackers on terrible Man yanited performance mmm");
    Task* rejectBranch=rejectBuilder.getResult();

    FieldValueRoutingStrategy router("outcome");
    DecisionTaskBuilder decisionBuilder;
    decisionBuilder.setBasicInfo("", "Review SIr furgasons opps");
    decisionBuilder.setRouter(&router);
    decisionBuilder.addBranchTarget("approve",approveBranch);
    decisionBuilder.addBranchTarget("reject" , rejectBranch);

    DecisionTask* decision = decisionBuilder.getDecisionResult();

    assert(decision!=nullptr);
    assert(decision->getBranches().size()==2);
    assert(decision->decide(nlohmann::json{{"outcome","approve"}})=="approve");

    delete decision;
    delete approveBranch;

    delete rejectBranch;

    TaskBuilder directorTaskBuilder;
    ProjectBuilder directorProjectBuilder;
    Director director;

    director.registerBuilder("Task", &directorTaskBuilder);

    director.registerBuilder("Project", &directorProjectBuilder);

    Task* directedTask=director.createNewTask("Book Old tradf");
    assert(directedTask!=nullptr);
    delete directedTask;


    WorkflowFactory factory;
    Task* factoryTask=factory.newTask("Prepare Ticket");
    assert(factoryTask!=nullptr);
    delete factoryTask;

    std::cout<< "You won the prem mate"<<std::endl;
    return 0;
}