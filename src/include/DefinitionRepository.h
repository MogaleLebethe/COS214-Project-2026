#ifndef TIVIDY_DEFINITION_REPOSITORY_H
#define TIVIDY_DEFINITION_REPOSITORY_H

#include <string>
#include <vector>
#include <map>

// Forward declaration
class WorkflowDefinition;

/**
 * @brief Repository storing reusable workflow definitions and template blueprints.
 * 
 * Maintains a registry of template definitions added by core systems or industry modules, 
 * allowing client code to spawn independent runtime instances on demand.
 */
class DefinitionRepository {
private:
    std::map<std::string, WorkflowDefinition> definitions; ///< Storage map keyed by definition ID.

public:
    /**
     * @brief Adds or registers a workflow definition blueprint.
     * 
     * @param def The workflow definition object to store.
     */
    void add(const WorkflowDefinition& def);

    /**
     * @brief Finds and returns a workflow definition by its unique identifier.
     * 
     * @param id The definition identifier.
     * @return `const WorkflowDefinition&` Const reference to the stored definition.
     * @throws `NotFoundError` If the definition ID does not exist.
     */
    const WorkflowDefinition& find(const std::string& id) const;

    /**
     * @brief Lists all available workflow definitions in the repository.
     * 
     * @return `std::vector<WorkflowDefinition>` A collection of all registered workflow definitions.
     */
    std::vector<WorkflowDefinition> list() const;
};

#endif // TIVIDY_DEFINITION_REPOSITORY_H