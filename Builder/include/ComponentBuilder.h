#pragma once
#include <string>

class Participant;
class DateTime;
class WorkflowComponent;
class Task;

class ComponentBuilder {
public:
    virtual ~ComponentBuilder();

    virtual void reset()=0;

    virtual void setBasicInfo(const std::string&id,const std::string& name)=0;
    virtual void setDescription(const std::string &description) =0;
    virtual void setExecutor(Participant*executor)=0;
    virtual void setDeadline(DateTime*deadline)=0;

    virtual void addDependency(WorkflowComponent*component)=0;
    virtual void setCustomField(const std::string& key,const std::string&value)=0;
    virtual void addWorkItem(Task*task) =0;
};