#pragma once
#include <stdexcept>
#include <string>

class WorkflowError : public std::runtime_error {
public:
    explicit WorkflowError(const std::string& message) : std::runtime_error(message) {}
};

class InvalidTransitionError : public WorkflowError {
public:
    explicit InvalidTransitionError(const std::string& message) : WorkflowError(message) {}
};

class NotFoundError : public WorkflowError {
public:
    explicit NotFoundError(const std::string& message) : WorkflowError(message) {}
};

class CyclicDependencyError : public WorkflowError {
public:
    explicit CyclicDependencyError(const std::string& message) : WorkflowError(message) {}
};
