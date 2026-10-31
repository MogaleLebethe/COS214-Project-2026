#ifndef TIVIDY_TASK_H
#define TIVIDY_TASK_H

#include "WorkflowComponent.h"
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

// Forward declarations
class TaskState;
class TaskMemento;
class WorkItemView;

/**
 * @brief Represents a concrete executable work item within the workflow tree.
 */
class Task : public WorkflowComponent {
private:
    static int counter;     ///< Static counter used for generating sequential task IDs.
    TaskState* currentState; ///< Non-owning raw pointer to the current lifecycle state flyweight.

public:
    /**
     * @brief Constructs a new Task with a generated unique identifier and default `PendingState`.
     */
    Task();

    /**
     * @brief Copy constructor for prototype-based cloning.
     */
    Task(const Task& other);

    /**
     * @brief Virtual destructor for the task.
     */
    virtual ~Task();

    /**
     * @brief Generates a unique string identifier for a new task instance.
     */
    static std::string generateId();

    /**
     * @brief Triggers the start action on the task.
     */
    void start();

    /**
     * @brief Triggers the completion action on the task.
     */
    void complete();

    /**
     * @brief Triggers the blocking action on the task.
     */
    void block();

    /**
     * @brief Triggers the failure action on the task.
     */
    void fail();

    /**
     * @brief Triggers the cancellation action on the task.
     */
    void cancel();

    /**
     * @brief Triggers the skipping action on the task.
     */
    void skip();

    /**
     * @brief Retrieves a raw pointer to the current lifecycle state object.
     */
    TaskState* getState() const;

    /**
     * @brief Sets the current lifecycle state of the task directly.
     */
    void setState(TaskState* state);

    /**
     * @brief Creates a memento snapshot capturing the task's current mutable data fields.
     */
    TaskMemento* createMemento() const;

    /**
     * @brief Restores the task's mutable fields from a provided memento snapshot.
     */
    void restoreMemento(const TaskMemento* m);

    /**
     * @brief Evaluates whether prerequisite dependencies are satisfied.
     */
    void evaluateDependencies() override;

    /**
     * @brief Checks whether the task has completed its execution lifecycle.
     */
    bool isCompleted() const override;

    /**
     * @brief Generates a textual summary string of the task's current status.
     */
    std::string getStatusSummary() const override;

    /**
     * @brief Collects actionable work items into a status data transfer view list.
     */
    void collectWork(std::vector<WorkItemView>& out) const override;

    /**
     * @brief Serialises the task into a structured JSON representation.
     */
    nlohmann::json toJson() const override;

    /**
     * @brief Creates a deep-copy prototype clone of the task with a fresh identifier.
     */
    WorkflowComponent* clone() const override;
};

#endif // TIVIDY_TASK_H