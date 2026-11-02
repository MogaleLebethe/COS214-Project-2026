#ifndef TIVIDY_TASK_STATE_H
#define TIVIDY_TASK_STATE_H

#include <string>

// Forward declarations
class Task;

/**
 * @brief Abstract base state interface implementing the State design pattern for tasks.
 * 
 * Defines lifecycle transition actions and status query hooks for managing 
 * the execution flow of a `Task`. Concretely implemented via flyweight singletons.
 */
class TaskState {
public:
    /**
     * @brief Virtual destructor for the task state interface.
     */
    virtual ~TaskState() = default;

    /**
     * @brief Triggers the start transition on a task.
     * 
     * @param task Non-owning raw pointer to the target `Task` instance.
     * @throws `WorkflowError` If the state transition is invalid.
     */
    virtual void start(Task* task) = 0;

    /**
     * @brief Triggers the completion transition on a task.
     * 
     * @param task Non-owning raw pointer to the target `Task` instance.
     * @throws `WorkflowError` If the state transition is invalid.
     */
    virtual void complete(Task* task) = 0;

    /**
     * @brief Triggers the blocking transition on a task.
     * 
     * @param task Non-owning raw pointer to the target `Task` instance.
     * @throws `WorkflowError` If the state transition is invalid.
     */
    virtual void block(Task* task) = 0;

    /**
     * @brief Triggers the failure transition on a task.
     * 
     * @param task Non-owning raw pointer to the target `Task` instance.
     * @throws `WorkflowError` If the state transition is invalid.
     */
    virtual void fail(Task* task) = 0;

    /**
     * @brief Triggers the cancellation transition on a task.
     * 
     * @param task Non-owning raw pointer to the target `Task` instance.
     * @throws `WorkflowError` If the state transition is invalid.
     */
    virtual void cancel(Task* task) = 0;

    /**
     * @brief Triggers the skipping transition on a task.
     * 
     * @param task Non-owning raw pointer to the target `Task` instance.
     * @throws `WorkflowError` If the state transition is invalid.
     */
    virtual void skip(Task* task) = 0;

    /**
     * @brief Retrieves the string representation of the state.
     * 
     * @return `std::string` The name of the state.
     */
    virtual std::string toString() const = 0;

    /**
     * @brief Checks whether the state satisfies completion criteria.
     * 
     * @return `bool` `true` if satisfied, `false` otherwise.
     */
    virtual bool isSatisfied() const = 0;
};

/**
 * @brief Concrete flyweight state representing a pending, unstarted task.
 */
class PendingState : public TaskState {
public:
    /**
     * @brief Retrieves the singleton instance of `PendingState`.
     * 
     * @return `TaskState*` Non-owning raw pointer to the singleton instance.
     */
    static TaskState* instance();

    void start(Task* task) override;
    void complete(Task* task) override;
    void block(Task* task) override;
    void fail(Task* task) override;
    void cancel(Task* task) override;
    void skip(Task* task) override;
    std::string toString() const override;
    bool isSatisfied() const override;

    PendingState(const PendingState&) = delete;
    PendingState& operator=(const PendingState&) = delete;

private:
    PendingState() = default;
};

/**
 * @brief Concrete flyweight state representing an active, in-progress task.
 */
class InProgressState : public TaskState {
public:
    /**
     * @brief Retrieves the singleton instance of `InProgressState`.
     * 
     * @return `TaskState*` Non-owning raw pointer to the singleton instance.
     */
    static TaskState* instance();

    void start(Task* task) override;
    void complete(Task* task) override;
    void block(Task* task) override;
    void fail(Task* task) override;
    void cancel(Task* task) override;
    void skip(Task* task) override;
    std::string toString() const override;
    bool isSatisfied() const override;

    InProgressState(const InProgressState&) = delete;
    InProgressState& operator=(const InProgressState&) = delete;

private:
    InProgressState() = default;
};

/**
 * @brief Concrete flyweight state representing a blocked task awaiting external resolution.
 */
class BlockedState : public TaskState {
public:
    /**
     * @brief Retrieves the singleton instance of `BlockedState`.
     * 
     * @return `TaskState*` Non-owning raw pointer to the singleton instance.
     */
    static TaskState* instance();

    void start(Task* task) override;
    void complete(Task* task) override;
    void block(Task* task) override;
    void fail(Task* task) override;
    void cancel(Task* task) override;
    void skip(Task* task) override;
    std::string toString() const override;
    bool isSatisfied() const override;

    BlockedState(const BlockedState&) = delete;
    BlockedState& operator=(const BlockedState&) = delete;

private:
    BlockedState() = default;
};

/**
 * @brief Concrete flyweight state representing a successfully completed task.
 */
class CompletedState : public TaskState {
public:
    /**
     * @brief Retrieves the singleton instance of `CompletedState`.
     * 
     * @return `TaskState*` Non-owning raw pointer to the singleton instance.
     */
    static TaskState* instance();

    void start(Task* task) override;
    void complete(Task* task) override;
    void block(Task* task) override;
    void fail(Task* task) override;
    void cancel(Task* task) override;
    void skip(Task* task) override;
    std::string toString() const override;
    bool isSatisfied() const override;

    CompletedState(const CompletedState&) = delete;
    CompletedState& operator=(const CompletedState&) = delete;

private:
    CompletedState() = default;
};

/**
 * @brief Concrete flyweight state representing a failed task execution.
 */
class FailedState : public TaskState {
public:
    /**
     * @brief Retrieves the singleton instance of `FailedState`.
     * 
     * @return `TaskState*` Non-owning raw pointer to the singleton instance.
     */
    static TaskState* instance();

    void start(Task* task) override;
    void complete(Task* task) override;
    void block(Task* task) override;
    void fail(Task* task) override;
    void cancel(Task* task) override;
    void skip(Task* task) override;
    std::string toString() const override;
    bool isSatisfied() const override;

    FailedState(const FailedState&) = delete;
    FailedState& operator=(const FailedState&) = delete;

private:
    FailedState() = default;
};

/**
 * @brief Concrete flyweight state representing a cancelled task.
 */
class CancelledState : public TaskState {
public:
    /**
     * @brief Retrieves the singleton instance of `CancelledState`.
     * 
     * @return `TaskState*` Non-owning raw pointer to the singleton instance.
     */
    static TaskState* instance();

    void start(Task* task) override;
    void complete(Task* task) override;
    void block(Task* task) override;
    void fail(Task* task) override;
    void cancel(Task* task) override;
    void skip(Task* task) override;
    std::string toString() const override;
    bool isSatisfied() const override;

    CancelledState(const CancelledState&) = delete;
    CancelledState& operator=(const CancelledState&) = delete;

private:
    CancelledState() = default;
};

/**
 * @brief Concrete flyweight state representing an explicitly skipped task.
 */
class SkippedState : public TaskState {
public:
    /**
     * @brief Retrieves the singleton instance of `SkippedState`.
     * 
     * @return `TaskState*` Non-owning raw pointer to the singleton instance.
     */
    static TaskState* instance();

    void start(Task* task) override;
    void complete(Task* task) override;
    void block(Task* task) override;
    void fail(Task* task) override;
    void cancel(Task* task) override;
    void skip(Task* task) override;
    std::string toString() const override;
    bool isSatisfied() const override;

    SkippedState(const SkippedState&) = delete;
    SkippedState& operator=(const SkippedState&) = delete;

private:
    SkippedState() = default;
};


#endif // TIVIDY_TASK_STATE_H