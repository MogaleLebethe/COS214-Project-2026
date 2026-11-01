#include "WorkflowEngineFacade.h"
#include "WorkflowFactory.h"
#include "ProjectRegistry.h"
#include "ActionRegistry.h"
#include "DefinitionRepository.h"
#include "Project.h"
#include "Support.h"
#include "WorkflowError.h"
#include "Task.h"
#include "Command.h"
#include "DecisionTask.h"

WorkflowEngineFacade::WorkflowEngineFacade(ProjectRegistry &reg, WorkflowFactory &f, ReportService &r, DefinitionRepository &d, ActionRegistry &a, RouterRegistry &rr)
    : registry(reg), factory(f), reports(r), definitions(d), actions(a), routers(rr) {}

std::string WorkflowEngineFacade::createProject(std::string name, std::string desc)
{
    Project* project = factory.newProject(name, desc);
    if (!project)
        throw WorkflowError("Project creation failed");
    
    try {
        registry.add(project);
    } catch (...) {
        delete project; // Clean up if registry insertion fails (e.g., duplicate ID)
        throw;
    }
    
    return project->getId();
}

std::string WorkflowEngineFacade::createFromTemplate(const nlohmann::json &templateJson)
{
    WorkflowComponent* w = factory.fromTemplate(templateJson);
    if (!w)
        throw WorkflowError("Template parsing failed");
        
    Project* project = dynamic_cast<Project*>(w);
    if (!project) {
        delete w; // Prevent leak if template root isn't a project
        throw WorkflowError("Root component of template must be a Project");
    }
    
    try {
        registry.add(project);
    } catch (...) {
        delete project; // Clean up if registry insertion fails
        throw;
    }
    
    return project->getId();
}

std::string WorkflowEngineFacade::startInstance(std::string definitionId, std::string instanceName)
{
    WorkflowDefinition def = definitions.find(definitionId);
    nlohmann::json templateJson = def.getTemplate();

    WorkflowComponent* w = factory.fromTemplate(templateJson);
    if (!w)
        throw WorkflowError("Template parsing failed");
        
    Project* project = dynamic_cast<Project*>(w);
    if (!project) {
        delete w; // Prevent leak if template root isn't a project
        throw WorkflowError("Root component of template must be a Project");
    }
    
    project->setName(instanceName);
    
    try {
        registry.add(project);
    } catch (...) {
        delete project; // Clean up if registry insertion fails
        throw;
    }
    
    return project->getId();
}

std::string WorkflowEngineFacade::addTaskToProject(std::string projectId, std::string name, std::string desc)
{
    Project& projRef = registry.findProject(projectId);
    Project* proj = &projRef;

    Task* task = factory.newTask(name, desc);
    if (!task) {
        throw WorkflowError("Failed to create task: " + name);
    }

    Command* cmd = nullptr;
    try {
        cmd = new AddWorkItemCommand(proj, task);
    } catch (...) {
        delete task; 
        throw;
    }

    CommandHistory& hist = registry.historyOf(projectId);
    try {
        hist.executeCommand(cmd); // CommandHistory takes ownership of `cmd`, which takes ownership of `task`
    } catch (...) {
        delete cmd;
        throw;
    }
    
    return task->getId();
}

void WorkflowEngineFacade::addDependency(std::string projectId, std::string dependentId, std::string prerequisiteId)
{
    WorkflowComponent& dep = registry.findComponent(projectId, dependentId);
    WorkflowComponent& prereq = registry.findComponent(projectId, prerequisiteId);
    
    Command* cmd = nullptr;
    try {
        cmd = new AddDependencyCommand(&dep, &prereq);
    } catch (...) {
        throw;
    }

    CommandHistory& hist = registry.historyOf(projectId);
    try {
        hist.executeCommand(cmd);
    } catch (...) {
        delete cmd;
        throw;
    }
}

void WorkflowEngineFacade::assignExecutor(std::string projectId, std::string taskId, const Participant &p)
{
    Task& task = registry.findTask(projectId, taskId);
    nlohmann::json payload = {{ "executorId", p.getId()}};
    editTask(projectId, taskId, payload);
    task.addParticipant(p);
}

std::vector<std::string> WorkflowEngineFacade::listProjects() const
{
    return registry.listIds();
}

void WorkflowEngineFacade::performAction(std::string projectId, std::string taskId, std::string action, const nlohmann::json & args)
{
    Task& task = registry.findTask(projectId, taskId);
    Command* cmd = nullptr;
    try {
        cmd = actions.create(action, &task, args);
    } catch (...) {
        throw;
    }

    CommandHistory& hist = registry.historyOf(projectId);
    try {
        hist.executeCommand(cmd);
    } catch (...) {
        delete cmd;
        throw;
    }
}

void WorkflowEngineFacade::editTask(std::string projectId, std::string taskId, const nlohmann::json &payload)
{
    Task* task = &registry.findTask(projectId, taskId);
    Command* cmd = nullptr;
    try {
        cmd = new EditTaskInfoCommand(task, payload);
    } catch (...) {
        throw;
    }

    CommandHistory& hist = registry.historyOf(projectId);
    try {
        hist.executeCommand(cmd);
    } catch (...) {
        delete cmd;
        throw;
    }
}

std::string WorkflowEngineFacade::decide(std::string projectId, std::string taskId, const nlohmann::json &context)
{
    WorkflowComponent& comp = registry.findComponent(projectId, taskId);
    DecisionTask* decisionTaskPtr = dynamic_cast<DecisionTask*>(&comp);
    if (!decisionTaskPtr)
        throw WorkflowError("Target task is not a DecisionTask");
    
    Command* cmd = nullptr;
    try {
        cmd = new DecideCommand(decisionTaskPtr, context);
    } catch (...) {
        throw;
    }

    CommandHistory& hist = registry.historyOf(projectId);
    try {
        hist.executeCommand(cmd);
    } catch (...) {
        delete cmd;
        throw;
    }
    
    return decisionTaskPtr->getSelectedBranch();
}

void WorkflowEngineFacade::undo(std::string projectId)
{
    CommandHistory& hist = registry.historyOf(projectId);
    hist.undo();
}

void WorkflowEngineFacade::redo(std::string projectId)
{
    CommandHistory& hist = registry.historyOf(projectId);
    hist.redo();
}

bool WorkflowEngineFacade::canUndo(std::string projectId) const
{
    const CommandHistory& hist = registry.historyOf(projectId);
    return hist.canUndo();
}

bool WorkflowEngineFacade::canRedo(std::string projectId) const
{
    const CommandHistory& hist = registry.historyOf(projectId);
    return hist.canRedo();
}

std::string WorkflowEngineFacade::viewProjectStatus(std::string projectId) const
{
    const Project& proj = registry.findProject(projectId);
    return reports.statusOf(proj);
}

nlohmann::json WorkflowEngineFacade::exportProjectToJson(std::string projectId) const
{
    const Project& proj = registry.findProject(projectId);
    return reports.exportJson(proj);
}

std::vector<WorkItemView> WorkflowEngineFacade::listAvailableWork(std::string projectId) const
{
    const Project& proj = registry.findProject(projectId);
    return reports.availableWork(proj);
}

std::vector<std::string> WorkflowEngineFacade::getHistory(std::string projectId) const
{
    const AuditLog& audit = registry.auditOf(projectId);
    return audit.toStrings();
}