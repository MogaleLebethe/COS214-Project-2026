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
#include "RouterRegistry.h"
#include <cctype>
#include <cstdio>
#include <map>
#include <set>

namespace
{
    bool looksGenerated(const std::string &id)
    {
        if (id.size() < 3 || (id[0] != 'T' && id[0] != 'P') || id[1] != '-')
            return false;
        for (std::size_t i = 2; i < id.size(); ++i)
        {
            if (!std::isdigit(static_cast<unsigned char>(id[i])))
                return false;
        }
        return true;
    }

    bool isUnsetDeadline(const std::string &text)
    {
        return text.empty() || text.compare(0, 4, "0000") == 0;
    }

    bool parseDeadlineText(const std::string &text, int &y, int &mo, int &d, int &h, int &mi)
    {
        h = 0;
        mi = 0;
        int got = std::sscanf(text.c_str(), "%d-%d-%d %d:%d", &y, &mo, &d, &h, &mi);
        return got == 3 || got == 5;
    }

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

std::string WorkflowEngineFacade::duplicateTask(std::string projectId, std::string taskId)
{
    Project &project = registry.findProject(projectId);
    Task &source = registry.findTask(projectId, taskId);
    if (dynamic_cast<DecisionTask *>(&source))
        throw WorkflowError("Decision tasks cannot be duplicated: their branches would not be copied");

    WorkflowComponent *copy = source.clone();
    std::string id = copy->getId();
    copy->setName(source.getName() + " (copy)");
    registry.execute(projectId, new AddWorkItemCommand(&project, copy));
    return id;
}

void WorkflowEngineFacade::removeTask(std::string projectId, std::string taskId)
{
    Project &project = registry.findProject(projectId);
    if (taskId == projectId)
        throw WorkflowError("A project cannot be removed from itself");
    WorkflowComponent &item = registry.findComponent(projectId, taskId);
    registry.execute(projectId, new RemoveWorkItemCommand(&project, &item));
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

bool WorkflowEngineFacade::isValidDateTime(int year, int month, int day, int hour, int minute)
{
    if (year < 1900 || year > 9999 || month < 1 || month > 12 || day < 1 ||
        hour < 0 || hour > 23 || minute < 0 ||minute > 59)
        return false;
    static const int daysInMonth[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    int maxDay = daysInMonth[month - 1];
    bool leap = (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
    if (month == 2 && leap)
        maxDay = 29;
    return day <= maxDay;
}

std::string WorkflowEngineFacade::instantiateDefinition(std::string definitionId, std::string instanceName)
{
    nlohmann::json tpl = definitions.find(definitionId).getTemplate();
    if (!instanceName.empty())
        tpl["name"] = instanceName;
    return importProject(tpl);
}

std::string WorkflowEngineFacade::importProject(const nlohmann::json &source)
{
    nlohmann::json root = source;
    if (root.is_object() && root.contains("project") && root["project"].is_object())
        root = root["project"]; 
    if (!root.is_object() || root.value("type", std::string()) != "Project")
        throw WorkflowError("Import needs a project exported from this engine (a JSON object with type \"Project\")");

    nlohmann::json items = nlohmann::json::array();
    if (root.contains("workItems"))
    {
        if (!root["workItems"].is_array())
            throw WorkflowError("Import: workItems must be an array");
        items = root["workItems"];
    }

    nlohmann::json tpl = root;
    tpl.erase("id");
    nlohmann::json cleaned = nlohmann::json::array();
    std::set<std::string> seen;
    for (std::size_t i = 0; i < items.size(); ++i)
    {
        const nlohmann::json &item = items[i];
        if (!item.is_object())
            throw WorkflowError("Import: every work item must be a JSON object");
        std::string type = item.value("type", std::string());
        
        if (type != "Task" && type != "Decision")
            throw WorkflowError("Import: unsupported work item type \"" + type + "\" (Task and Decision only)");

        std::string oldId = item.value("id", std::string());
        if (!oldId.empty() && !seen.insert(oldId).second)
            throw WorkflowError("Import: duplicate id " + oldId);

        nlohmann::json c = item;
        if (oldId.empty() || looksGenerated(oldId))
            c.erase("id");

        if (c.contains("executor") && (!c["executor"].is_object() || c["executor"].value("id", std::string()).empty()))
            c.erase("executor"); 

        c.erase("router");      
        c.erase("workItems");
        cleaned.push_back(c);
    }
    tpl["workItems"] = cleaned;

    Project *project = asProject(factory.fromTemplate(tpl), "Import");
    try
    {
        const std::vector<WorkflowComponent *> &built = project->getWorkItems();
        if (built.size() != items.size())
            throw WorkflowError("Import: could not rebuild every work item");

        std::map<std::string, WorkflowComponent *> byOldId;
        for (std::size_t i = 0; i < items.size(); ++i)
        {
            std::string oldId = items[i].value("id", std::string());
            if (!oldId.empty())
                byOldId[oldId] = built[i];
        }

        for (std::size_t i = 0; i < items.size(); ++i)
        {
            const nlohmann::json &item = items[i];
            WorkflowComponent *component = built[i];

            if (item.contains("deadline") && item["deadline"].is_string())
            {
                std::string text = item["deadline"].get<std::string>();
                if (!isUnsetDeadline(text))
                {
                    int y = 0, mo = 0, d = 0, h = 0, mi = 0;
                    if (!parseDeadlineText(text, y, mo, d, h, mi) || !isValidDateTime(y, mo, d, h, mi))
                        throw WorkflowError("Import: invalid deadline \"" + text + "\" on " + component->getName());
                    component->setDeadline(DateTime(y, mo, d, h, mi));
                }
            }

            if (item.contains("dependsOn") && item["dependsOn"].is_array())
            {
                for (std::size_t k = 0; k < item["dependsOn"].size(); ++k)
                {
                    std::string prerequisite = item["dependsOn"][k].get<std::string>();
                    std::map<std::string, WorkflowComponent *>::const_iterator found = byOldId.find(prerequisite);
                    if (found == byOldId.end())
                        throw WorkflowError("Import: " + component->getName() + " waits for unknown id " + prerequisite);
                    component->addDependency(found->second);
                }
            }
        }

        for (std::size_t i = 0; i < items.size(); ++i)
        {
            if (items[i].value("type", std::string()) != "Decision")
                continue;
            DecisionTask *decision = dynamic_cast<DecisionTask *>(built[i]);
            if (!decision)
                throw WorkflowError("Import: could not rebuild decision " + built[i]->getName());

            std::string key = items[i].value("router", std::string());
            if (key.empty() && items[i].contains("customFields") && items[i]["customFields"].is_object())
                key = items[i]["customFields"].value("router", std::string());
            if (!key.empty())
            {
                decision->setRouter(routers.get(key)); //Note that this throws NotFoundError if router isn't registered
                decision->setCustomField("router", key);
            }

            if (items[i].contains("branches") && items[i]["branches"].is_object())
            {
                for (nlohmann::json::const_iterator b = items[i]["branches"].begin(); b != items[i]["branches"].end(); ++b)
                {
                    for (std::size_t k = 0; k < b.value().size(); ++k)
                    {
                        std::string targetId = b.value()[k].get<std::string>();
                        std::map<std::string, WorkflowComponent *>::const_iterator found = byOldId.find(targetId);
                        Task *target = (found == byOldId.end()) ? nullptr : dynamic_cast<Task *>(found->second);
                        if (!target)
                            throw WorkflowError("Import: branch \"" + b.key() + "\" points at unknown task " + targetId);
                        decision->addBranchTarget(b.key(), target);
                    }
                }
            }
        }
    }
    catch (...)
    {
        delete project;
        throw;
    }
    return registerProject(registry, project);
}

nlohmann::json WorkflowEngineFacade::listDefinitions() const
{
    nlohmann::json out = nlohmann::json::array();
    std::vector<WorkflowDefinition> defs = definitions.list();
    for (std::size_t i = 0; i < defs.size(); ++i)
    {
        nlohmann::json d;
        d["id"] = defs[i].getId();
        d["name"] = defs[i].getName();
        d["industry"] = defs[i].getIndustry();
        d["description"] = defs[i].getDescription();
        out.push_back(d);
    }
    return out;
}

void WorkflowEngineFacade::performAction(std::string projectId, std::string taskId, std::string action, const nlohmann::json &args)
{
    Task &task = registry.findTask(projectId, taskId);
    registry.execute(projectId, actions.create(action, &task, args)); 
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

std::vector<std::string> WorkflowEngineFacade::listActions() const
{
    return actions.names();
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
