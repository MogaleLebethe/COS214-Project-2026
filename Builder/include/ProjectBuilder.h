#pragma once
#include "ComponentBuilder.h"
class Project;

class ProjectBuilder : public ComponentBuilder {
public:
    ProjectBuilder();
    virtual ~ProjectBuilder();

    void reset()override;

    void setBasicInfo(const std::string&id, const std::string& name)override;
    void setDescription(const std::string&description) override;
    void setExecutor(Participant*executor) override;
    void setDeadline(DateTime*deadline) override;

    void addDependency(WorkflowComponent* component) override;
    void setCustomField(const std::string&key, const std::string & value) override;


    void addWorkItem(Task* task) override;

    Project* getResult();

private:
    Project* product_;
};