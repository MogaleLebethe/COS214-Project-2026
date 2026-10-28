#pragma once
#include "ComponentBuilder.h"
class Task;

class TaskBuilder : public ComponentBuilder {
public:
    TaskBuilder();
    virtual ~TaskBuilder();

    void reset() override;

    void setBasicInfo(const std::string&id,const std::string&name)override;
    void setDescription(const std::string&description)override;
    void setExecutor(Participant*executor) override;
    void setDeadline(DateTime*deadline) override;

    void addDependency(WorkflowComponent*component)override;
    void setCustomField(const std::string& key,const std::string&value)override;
    void addWorkItem(Task *task)override;
    Task* getResult();

private:
    Task* product_;
};