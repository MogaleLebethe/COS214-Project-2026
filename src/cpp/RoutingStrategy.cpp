/**
 * @file RoutingStrategy.cpp
 * @brief Implementations of RoutingStrategy and FieldValueRoutingStrategy.
 */
#include "RoutingStrategy.h"
#include "WorkflowError.h"

RoutingStrategy::~RoutingStrategy() {}

// FieldValueRoutingStrategy

FieldValueRoutingStrategy::FieldValueRoutingStrategy(const std::string& field) : field(field) {}

std::string FieldValueRoutingStrategy::route(const DecisionTask&, const nlohmann::json& context) const {
    if (!context.is_object() || !context.contains(field))
        throw NotFoundError("Routing field '" + field + "' missing from decision context");

    const nlohmann::json& value = context[field];
    return value.is_string() ? value.get<std::string>() : value.dump();
}

std::string FieldValueRoutingStrategy::getField() const {
    return field;
}
