#ifndef TIVIDY_ACTION_REGISTRY_H
#define TIVIDY_ACTION_REGISTRY_H

#include <string>
#include <vector>
#include <functional>
#include <map>
#include <nlohmann/json.hpp>

// Forward declarations
class Command;
class Task;

/**
 * @brief Type definition for action factory functions (`ActionCreator`).
 * 
 * Takes a target task raw pointer and a JSON arguments payload, returning a newly allocated concrete Command pointer.
 */
using ActionCreator = std::function<Command*(Task*, const nlohmann::json&)>;

/**
 * @brief Registry mapping action string names to command creation lambdas/factories.
 * 
 * Allows built-in actions and industry-specific extensions (e.g., approve, reject, escalate) 
 * to be dynamically registered and looked up without modifying core engine logic.
 */
class ActionRegistry {
private:
    std::map<std::string, ActionCreator> creators; ///< Map linking action names to their creation callbacks.

public:
    /**
     * @brief Constructs the registry and pre-loads standard built-in actions (start, complete, block, fail, cancel).
     */
    ActionRegistry();

    /**
     * @brief Registers a custom action creator function under a unique name string.
     * 
     * @param name The string identifier for the action (e.g., "approve").
     * @param creator The `ActionCreator` factory function or lambda.
     */
    void registerAction(std::string name, ActionCreator creator);

    /**
     * @brief Instantiates a command object by looking up its action creator name.
     * 
     * @param name The string identifier of the action.
     * @param task Raw pointer to the target task receiving the action.
     * @param args JSON arguments payload passed to the creator.
     * @return `Command*` Raw pointer to the newly allocated concrete command object.
     * @throws `NotFoundError` If the action name is not registered.
     */
    Command* create(const std::string& name, Task* task, const nlohmann::json& args) const;

    /**
     * @brief Checks if an action name is currently registered.
     * 
     * @param name The string identifier of the action.
     * @return `bool` `true` If the action exists, `false` otherwise.
     */
    bool has(const std::string& name) const;

    /**
     * @brief Retrieves a list of all registered action names.
     * 
     * @return `std::vector<std::string>` A collection of available action string identifiers.
     */
    std::vector<std::string> names() const;
};

#endif // TIVIDY_ACTION_REGISTRY_H
