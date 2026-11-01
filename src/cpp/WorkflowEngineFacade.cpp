#include "WorkflowEngineFacade.h"
#include "WorkflowFactory.h"
#include "ProjectRegistry.h"
#include "ActionRegistry.h"
#include "DefinitionRepository.h"
#include "Project.h"
#include "Support.h"
#include "WorkflowError.h"
#include "ReportService.h"
#include "Task.h"
#include "Command.h"
#include "DecisionTask.h"

namespace
{

    std::string registerProject(ProjectRegistry &registry, Project *project)
    {
        std::string id = project->getId();
        try
        {
            registry.add(project);
        }
        catch (...)
        {
            delete project;
            throw;
        }
        return id;
    }

    Project *asProject(WorkflowComponent *w, const char *what)
    {
        if (!w)
            throw WorkflowError(std::string(what) + " failed");
        Project *project = dynamic_cast<Project *>(w);
        if (!project)
        {
            delete w;
            throw WorkflowError("Root component must be a Project");
        }
        return project;
    }

}

WorkflowEngineFacade::WorkflowEngineFacade(ProjectRegistry &reg, WorkflowFactory &f, ReportService &r, DefinitionRepository &d, ActionRegistry &a, RouterRegistry &rr)
    : registry(reg), factory(f), reports(r), definitions(d), actions(a), routers(rr) {}

std::string WorkflowEngineFacade::createProject(std::string name, std::string desc)
{
    return registerProject(registry, factory.newProject(name, desc));
}

std::string WorkflowEngineFacade::createFromTemplate(const nlohmann::json &templateJson)
{
    return registerProject(registry, asProject(factory.fromTemplate(templateJson), "Template parsing"));
}

std::string WorkflowEngineFacade::startInstance(std::string definitionId, std::string instanceName)
{
    const WorkflowDefinition &def = definitions.find(definitionId);
    Project *project = asProject(factory.fromTemplate(def.getTemplate()), "Template parsing");
    project->setName(instanceName);
    return registerProject(registry, project);
}

std::string WorkflowEngineFacade::cloneProject(std::string projectId, std::string newName)
{
    const Project &source = registry.findProject(projectId);
    Project *copy = asProject(source.clone(), "Clone");
    copy->setName(newName);
    return registerProject(registry, copy);
}

std::string WorkflowEngineFacade::addTaskToProject(std::string projectId, std::string name, std::string desc)
{
    Project &project = registry.findProject(projectId);

    Task *task = factory.newTask(name, desc);
    std::string id = task->getId();
    Command *cmd = new AddWorkItemCommand(&project, task);
    registry.execute(projectId, cmd);
    return id;
}

void WorkflowEngineFacade::addDependency(std::string projectId, std::string dependentId, std::string prerequisiteId)
{
    WorkflowComponent &dep = registry.findComponent(projectId, dependentId);
    WorkflowComponent &prereq = registry.findComponent(projectId, prerequisiteId);
    registry.execute(projectId, new AddDependencyCommand(&dep, &prereq));
}
void WorkflowEngineFacade::assignExecutor(std::string projectId, std::string taskId, const Participant &p)
{
    nlohmann::json executor;
    executor["id"] = p.getId();
    executor["name"] = p.getName();
    executor["role"]= p.getRole();

    nlohmann::json payload;
    payload["executor"] = executor;
    editTask(projectId,taskId,payload);
}

std::vector<std::string> WorkflowEngineFacade::listProjects() const
{
    return registry.listIds();
}

void WorkflowEngineFacade::performAction(std::string projectId, std::string taskId, std::string action, const nlohmann::json &args)
{
    Task &task = registry.findTask(projectId, taskId);
    registry.execute(projectId, actions.create(action, &task, args)); // ownership -> registry
}

void WorkflowEngineFacade::editTask(std::string projectId, std::string taskId, const nlohmann::json &payload)
{
    Task &task = registry.findTask(projectId, taskId);
    registry.execute(projectId, new EditTaskInfoCommand(&task, payload));
}

std::string WorkflowEngineFacade::decide(std::string projectId, std::string taskId, const nlohmann::json &context)
{
    WorkflowComponent &comp = registry.findComponent(projectId, taskId);
    DecisionTask *decisionTask = dynamic_cast<DecisionTask *>(&comp);
    if (!decisionTask)
        throw WorkflowError("Target task is not a DecisionTask");

    registry.execute(projectId, new DecideCommand(decisionTask, context));
    return decisionTask->getSelectedBranch();
}

void WorkflowEngineFacade::undo(std::string projectId)
{
    registry.undo(projectId);
}

void WorkflowEngineFacade::redo(std::string projectId)
{
    registry.redo(projectId);
}

bool WorkflowEngineFacade::canUndo(std::string projectId) const
{
    return registry.historyOf(projectId).canUndo();
}

bool WorkflowEngineFacade::canRedo(std::string projectId) const
{
    return registry.historyOf(projectId).canRedo();
}

std::string WorkflowEngineFacade::viewProjectStatus(std::string projectId) const
{
    return reports.statusOf(registry.findProject(projectId));
}

nlohmann::json WorkflowEngineFacade::exportProjectToJson(std::string projectId) const
{
    return reports.exportJson(registry.findProject(projectId));
}

std::vector<WorkItemView> WorkflowEngineFacade::listAvailableWork(std::string projectId) const
{
    return reports.availableWork(registry.findProject(projectId));
}

std::vector<std::string> WorkflowEngineFacade::getHistory(std::string projectId) const
{
    return registry.auditOf(projectId).toStrings();
}
