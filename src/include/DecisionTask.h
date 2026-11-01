#ifndef TIVIDY_DECISION_TASK_H
#define TIVIDY_DECISION_TASK_H

#include "Task.h"
#include <string>
#include <vector>
#include <map>
#include <nlohmann/json.hpp>

// Forward declarations
class RoutingStrategy;
class Task;

/**
 * @brief Represents a specialized task that evaluates conditional branching logic.
 * 
 * Inherits from `Task` and manages a set of named branch targets mapped via 
 * a `RoutingStrategy` to dynamically select execution paths based on evaluation context.
 */
class DecisionTask : public Task {
private:
    RoutingStrategy* router;                            ///< Non-owning raw pointer to the routing strategy.
    std::map<std::string, std::vector<Task*> > branches; ///< Map of branch labels to target task vectors.
    std::string selectedBranch;                         ///< The label of the currently selected branch.

public:
    /**
     * @brief Constructs a new DecisionTask instance.
     */
    DecisionTask();

    /**
     * @brief Copy constructor for prototype-based cloning.
     * 
     * @param other Const reference to the source `DecisionTask` being copied.
     */
    DecisionTask(const DecisionTask& other);

    /**
     * @brief Virtual destructor for the decision task.
     */
    virtual ~DecisionTask();

    /**
     * @brief Sets the routing strategy used to evaluate branches.
     * 
     * @param r Non-owning raw pointer to the `RoutingStrategy`.
     */
    void setRouter(RoutingStrategy* r);

    /**
     * @brief Adds a target task to a specified branch label.
     * 
     * @param label Const reference to the branch label string.
     * @param target Raw pointer to the target `Task` to add.
     */
    void addBranchTarget(const std::string& label, Task* target);

    /**
     * @brief Retrieves the vector of target tasks associated with branch labels.
     * 
     * @return `std::vector<Task*>` List of target tasks.
     */
    std::vector<Task*> getBranchTargets() const;

    /**
     * @brief Evaluates the decision router using the provided context JSON payload.
     * 
     * @param context Const reference to the `nlohmann::json` evaluation context.
     * @return `std::string` The label of the selected branch.
     * @throws `WorkflowError` If evaluation fails or router is missing.
     */
    std::string decide(const nlohmann::json& context);

    /**
     * @brief Clears the current decision and resets branch selection state.
     */
    void clearDecision();

    /**
     * @brief Retrieves the label of the currently selected branch.
     * 
     * @return `std::string` The selected branch label string.
     */
    const std::string& getSelectedBranch() const;

    /**
     * @brief Retrieves the complete branch mapping table.
     * 
     * @return `const std::map<std::string, std::vector<Task*> >&` Reference to branch maps.
     */
    const std::map<std::string, std::vector<Task*> >& getBranches() const;

    /**
     * @brief Serializes the decision task into a structured JSON representation.
     * 
     * @return `nlohmann::json` The serialized JSON object.
     */
    nlohmann::json toJson() const override;

    /**
     * @brief Creates a deep-copy prototype clone of the decision task.
     * 
     * @return `WorkflowComponent*` Raw pointer to the newly allocated clone component.
     */
    WorkflowComponent* clone() const override;
};

#endif // TIVIDY_DECISION_TASK_H