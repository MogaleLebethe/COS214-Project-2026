#include "DefinitionRepository.h"
#include "Support.h"
#include "WorkflowError.h"

void DefinitionRepository::add(const WorkflowDefinition &def)
{
    definitions[def.getId()] = def;
}

const WorkflowDefinition &DefinitionRepository::find(const std::string &id) const
{
    auto it = definitions.find(id);
    if (it == definitions.end())
        throw NotFoundError("Workflow definition not found: " + id);
    return it->second;
}

std::vector<WorkflowDefinition> DefinitionRepository::list() const
{
    std::vector<WorkflowDefinition> list;
    list.reserve(definitions.size());

    for (const auto& item : definitions) 
    {
        list.push_back(item.second);
    }

    return list;
}
