#ifndef TIVIDY_WORKFLOW_ERROR_H
#define TIVIDY_WORKFLOW_ERROR_H

#include <stdexcept>
#include <string>

/**
 * @brief Base exception class for all workflow engine errors.
 * 
 * Extends `std::runtime_error` to provide domain-specific error handling 
 * across components, registries, and execution pipelines.
 */
class WorkflowError : public std::runtime_error {
public:
    /**
     * @brief Constructs a workflow error with a descriptive message.
     * 
     * @param message Descriptive error string.
     */
    explicit WorkflowError(const std::string& message);

    /**
     * @brief Virtual destructor.
     */
    virtual ~WorkflowError() noexcept;
};

/**
 * @brief Exception thrown when a requested entity, component, or project ID cannot be found.
 */
class NotFoundError : public WorkflowError {
public:
    /**
     * @brief Constructs a not found error with a descriptive message.
     * 
     * @param message Descriptive error string.
     */
    explicit NotFoundError(const std::string& message);
};

/**
 * @brief Exception thrown when an illegal task state transition is attempted.
 */
class InvalidTransitionError : public WorkflowError {
public:
    /**
     * @brief Constructs an invalid transition error with a descriptive message.
     * 
     * @param message Descriptive error string.
     */
    explicit InvalidTransitionError(const std::string& message);
};

/**
 * @brief Exception thrown when adding a dependency introduces a cyclic graph loop.
 */
class CyclicDependencyError : public WorkflowError {
public:
    /**
     * @brief Constructs a cyclic dependency error with a descriptive message.
     * 
     * @param message Descriptive error string.
     */
    explicit CyclicDependencyError(const std::string& message);
};

#endif // TIVIDY_WORKFLOW_ERROR_H