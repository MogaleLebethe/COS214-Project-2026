#include "WorkflowError.h"

WorkflowError::WorkflowError(const std::string& message)
    : std::runtime_error(message) {}

WorkflowError::~WorkflowError() noexcept = default;

NotFoundError::NotFoundError(const std::string& message)
    : WorkflowError(message) {}

InvalidTransitionError::InvalidTransitionError(const std::string& message)
    : WorkflowError(message) {}

CyclicDependencyError::CyclicDependencyError(const std::string& message)
    : WorkflowError(message) {}