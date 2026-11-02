#ifndef TIVIDY_TASK_STATE_FACTORY_H
#define TIVIDY_TASK_STATE_FACTORY_H

#include <string>

// Forward declarations
class TaskState;

/**
 * @brief Flyweight factory responsible for resolving and retrieving task state singletons.
 */
class TaskStateFactory {
public:
    /**
     * @brief Retrieves a task state singleton matching the specified string name.
     * 
     * @param name Const reference to the state identifier string (e.g., "Pending", "InProgress").
     * @return `TaskState*` Non-owning raw pointer to the corresponding state singleton.
     * @throws `NotFoundError` If the state name is unrecognized.
     */
    static TaskState* get(const std::string& name);
};

#endif // TIVIDY_TASK_STATE_FACTORY_H