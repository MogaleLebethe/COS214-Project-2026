/**
 * @file RoutingStrategy.h
 * @brief Strategy pattern: abstract RoutingStrategy and the generic FieldValueRoutingStrategy.
 */
#pragma once

#include <string>

#include "nlohmann/json.hpp"

class DecisionTask;

/**
 * @brief Abstract algorithm that chooses which branch a `DecisionTask` takes.
 *
 * Industries supply their own strategies so routing rules can change without
 * modifying the engine. Strategies are owned by `RouterRegistry`; a `DecisionTask`
 * only borrows its router.
 */
class RoutingStrategy {
public:
    /**
     * @brief Default constructor.
     */
    RoutingStrategy() {}

    /**
     * @brief Virtual destructor so concrete strategies are deleted correctly through a base pointer.
     */
    virtual ~RoutingStrategy();

    /**
     * @brief Chooses a branch for a decision.
     *
     * @param task The decision task being routed.
     * @param context JSON data describing the case (e.g. claim amount, assessment outcome).
     * @return `std::string` The label of the chosen branch.
     * @throws `NotFoundError` If the context does not contain what the strategy needs.
     */
    virtual std::string route(const DecisionTask& task, const nlohmann::json& context) const = 0;

private:
    RoutingStrategy(const RoutingStrategy&) = delete;
    RoutingStrategy& operator=(const RoutingStrategy&) = delete;
};

/**
 * @brief Generic strategy that routes on the value of one field in the context.
 *
 * With field "outcome", the context {"outcome": "approve"} routes to the "approve" branch.
 * Non-string values are converted to text, so {"approved": true} routes to "true".
 */
class FieldValueRoutingStrategy : public RoutingStrategy {
private:
    std::string field;  ///< Name of the context field that holds the branch label.

public:
    /**
     * @brief Constructs the strategy.
     *
     * @param field Name of the context field to read the branch label from.
     */
    explicit FieldValueRoutingStrategy(const std::string& field);

    /**
     * @brief Returns the value of the configured field as the branch label.
     *
     * @param task The decision task being routed (unused).
     * @param context JSON object that must contain the configured field.
     * @return `std::string` The field's value as text.
     * @throws `NotFoundError` If the context is not an object or the field is missing.
     */
    std::string route(const DecisionTask& task, const nlohmann::json& context) const override;

    /**
     * @brief Retrieves the configured field name.
     *
     * @return `std::string` The context field this strategy reads.
     */
    std::string getField() const;
};
