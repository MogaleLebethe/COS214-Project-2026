#include "ActionRegistry.h"
#include "WorkflowError.h"
#include "Command.h"
#include "Task.h"

ActionRegistry::ActionRegistry()
{
    registerAction("start", [](Task* task, const nlohmann::json&) -> Command* {
        return new TaskStateCommand(task, [](Task& t) { t.start(); }, "Start");
    });

    registerAction("complete", [](Task* task, const nlohmann::json&) -> Command* {
        return new TaskStateCommand(task, [](Task& t) { t.complete(); }, "Complete");
    });

    registerAction("block", [](Task* task, const nlohmann::json&) -> Command* {
        return new TaskStateCommand(task, [](Task& t) { t.block(); }, "Block");
    });

    registerAction("fail", [](Task* task, const nlohmann::json&) -> Command* {
        return new TaskStateCommand(task, [](Task& t) { t.fail(); }, "Fail");
    });

    registerAction("cancel", [](Task* task, const nlohmann::json&) -> Command* {
        return new TaskStateCommand(task, [](Task& t) { t.cancel(); }, "Cancel");
    });
}

void ActionRegistry::registerAction(const std::string& name, ActionCreator creator)
{
    if (!creator)
        throw WorkflowError("Cannot register action: creator function is null");
    creators[name] = creator;
}

Command *ActionRegistry::create(const std::string &name, Task *task, const nlohmann::json &args) const
{
    if (!task)
        throw WorkflowError("Cannot execute action on null task pointer");
    std::map<std::string, ActionCreator>::const_iterator it = creators.find(name);
    if (it == creators.end())
        throw NotFoundError("Action creator not registered: " + name);
    return it->second(task, args);
}

bool ActionRegistry::has(const std::string &name) const
{
    return creators.find(name) != creators.end();
}

std::vector<std::string> ActionRegistry::names() const
{
    std::vector<std::string> names;
    names.reserve(creators.size());

    for (std::map<std::string, ActionCreator>::const_iterator it = creators.begin(); it != creators.end(); ++it)
        names.push_back(it->first);

    return names;
}
