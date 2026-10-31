#pragma once
#include <string>

class Participant;
class DateTime;
class WorkflowComponent;

class ComponentBuilder {
public:
    virtual ~ComponentBuilder();

    virtual void reset() = 0;
    virtual void setBasicInfo(const std::string& d, const std::string&name) =0;
    virtual void setDescription(const std::string&description) =0;
    virtual void setExecutor(const Participant&executor) =0;
    virtual void setDeadline(const DateTime&deadline) =0;
    virtual void addDependency(WorkflowComponent*component) =0;
    virtual void addCustomField(const std::string&key, const std::string& value) =0;
    virtual void addWorkItem(WorkflowComponent*item) =0;

    virtual WorkflowComponent* build() =0;
};
