#include "ProjectRegistry.h"
#include "Support.h"
#include "Project.h"
#include "Task.h"
#include "WorkflowError.h"

ProjectRegistry::~ProjectRegistry()
{
    for (auto item : sessions) {
        if (item.second)
            delete item.second;
    }
    sessions.clear();
}

std::string ProjectRegistry::add(Project* p)
{
    if (!p) 
        throw WorkflowError("Project cannot be null");
    
    std::string id = p->getId();

    if (sessions.find(id) != sessions.end()) 
        throw WorkflowError("Project session already exists: " + id);
    

    ProjectSession* session = new ProjectSession(id, p);
    sessions[id] = session;
    return id;
}

Project &ProjectRegistry::findProject(const std::string &id) const
{
    auto it = sessions.find(id);
    if (it == sessions.end())
        throw NotFoundError("Project session not found: " + id);
    ProjectSession* session = it->second;
    Project* p = session->getActiveProject();
    if (!p) 
        throw WorkflowError("Active project at " + id + " is null");
    return *p;
}

Task &ProjectRegistry::findTask(const std::string &projectId, const std::string &taskId) const
{
    Project& project = findProject(projectId);
    WorkflowComponent* task = project.findById(taskId);
    if (!task)
        throw NotFoundError("Task not found in project: " + taskId);
    Task* taskPtr = dynamic_cast<Task*>(task);
    if (!taskPtr)
        throw WorkflowError("Component " + taskId + " is not a Task");
    return *taskPtr;
}

WorkflowComponent &ProjectRegistry::findComponent(const std::string &projectId, const std::string &componentId) const
{
    Project& project = findProject(projectId);
    WorkflowComponent* component = project.findById(projectId);
    if (!component)
        throw NotFoundError("Component not found: " + componentId);
    return *component;
}

CommandHistory &ProjectRegistry::historyOf(const std::string &projectId)
{
    auto it = sessions.find(projectId);
    if (it == sessions.end())
        throw NotFoundError("Project session not found: " + projectId);
    ProjectSession* session = it->second;
    CommandHistory* history = session->getCommandHistory();
    if (!history)
        throw WorkflowError("Command history is null for project: " + projectId);
    return *history;
}

const AuditLog &ProjectRegistry::auditOf(const std::string &projectId) const
{
    auto it = sessions.find(projectId);
    if (it == sessions.end())
        throw NotFoundError("Project session not found: " + projectId);
    ProjectSession* session = it->second;
    AuditLog* log = session->getAuditLog();
    if (!log)
        throw WorkflowError("Audit log is null for project: " + projectId);
    return *log;
}

std::vector<std::string> ProjectRegistry::listIds() const
{
    std::vector<std::string> list;
    list.reserve(sessions.size());

    for (const auto& item : sessions) 
    {
        list.push_back(item.first);
    }
    return list;
}
