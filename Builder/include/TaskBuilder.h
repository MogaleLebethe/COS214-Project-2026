#pragma once
#include "ComponentBuilder.h"

class Task;

class TaskBuilder : public ComponentBuilder {
protected:
    Task* product;

    TaskBuilder(const TaskBuilder&) = delete;
    TaskBuilder& operator=(const TaskBuilder&) = delete;

    void validate() const;

public:
    TaskBuilder();
    virtual ~TaskBuilder();

    void reset() override;
    void setBasicInfo(const std::string& id, const std::string&name) override;
    void setDescription(const std::string& description) override;
    void setExecutor(const Participant&executor) override;
    void setDeadline(const DateTime&deadline) override;
    void addDependency(WorkflowComponent*component) override;
    void addCustomField(const std::string&key, const std::string& value) override;
    void addWorkItem(WorkflowComponent*item) override;

    Task* getResult();
    WorkflowComponent* build()override;
};
