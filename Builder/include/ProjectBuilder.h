#pragma once
#include "ComponentBuilder.h"

class Project;

class ProjectBuilder : public ComponentBuilder {
protected:
    Project* product;

    ProjectBuilder(const ProjectBuilder&) = delete;
    ProjectBuilder& operator=(const ProjectBuilder&) = delete;

    void validate() const;

public:
    ProjectBuilder();
    virtual ~ProjectBuilder();

    void reset() override;
    void setBasicInfo(const std::string& id, const std::string& name) override;
    void setDescription(const std::string& description) override;
    void setExecutor(const Participant& executor) override;
    void setDeadline(const DateTime& deadline) override;
    void addDependency(WorkflowComponent* component) override;
    void addCustomField(const std::string& key, const std::string& value) override;
    void addWorkItem(WorkflowComponent* item) override;

    Project* getResult();
    WorkflowComponent* build() override;
};
