#include "ActionRegistry.h"
#include "WorkflowError.h"
#include "Command.h"
#include "Task.h"

ActionRegistry::ActionRegistry()
{
    registerAction("start", [](Task* task, const nlohmann::json& args) -> Command* {
        return new TaskStateCommand(task, [](Task& t) { t.start(); }, "Start Task");
    });

    registerAction("complete", [](Task* task, const nlohmann::json& args) -> Command* {
        return new TaskStateCommand(task, [](Task& t) { t.complete(); }, "Complete Task");
    });

    registerAction("block", [](Task* task, const nlohmann::json& args) -> Command* {
        return new TaskStateCommand(task, [](Task& t) { t.block(); }, "Block Task");
    });   

    registerAction("fail", [](Task* task, const nlohmann::json& args) -> Command* {
        return new TaskStateCommand(task, [](Task& t) { t.fail(); }, "Fail Task");
    });

    registerAction("cancel", [](Task* task, const nlohmann::json& args) -> Command* {
        return new TaskStateCommand(task, [](Task& t) { t.cancel(); }, "Cancel Task");
    });
}

ActionRegistry::ActionRegistry()
{
    registerAction("start", [](Task* task, const nlohmann::json& args) -> Command* {
        return new TaskStateCommand(task, [](Task& t) { t.start(); }, "Start Task");
    });

    registerAction("complete", [](Task* task, const nlohmann::json& args) -> Command* {
        return new TaskStateCommand(task, [](Task& t) { t.complete(); }, "Complete Task");
    });

    registerAction("block", [](Task* task, const nlohmann::json& args) -> Command* {
        return new TaskStateCommand(task, [](Task& t) { t.block(); }, "Block Task");
    });   

    registerAction("fail", [](Task* task, const nlohmann::json& args) -> Command* {
        return new TaskStateCommand(task, [](Task& t) { t.fail(); }, "Fail Task");
    });

    registerAction("cancel", [](Task* task, const nlohmann::json& args) -> Command* {
        return new TaskStateCommand(task, [](Task& t) { t.cancel(); }, "Cancel Task");
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
    if (!has(name))
        throw NotFoundError("Action creator not registered: " + name);
    if (!task)
        throw WorkflowError("Cannot execute action on null task pointer");
    ActionCreator c = creators.find(name)->second;
    return c(task, args);
}

bool ActionRegistry::has(const std::string &name) const
{
    return creators.find(name) != creators.end();
}

std::vector<std::string> ActionRegistry::names() const
{
    std::vector<std::string> names;
    names.reserve(creators.size());

    for (const auto& item : creators)
    {
        names.push_back(item.first);
    }

    return names;
}
