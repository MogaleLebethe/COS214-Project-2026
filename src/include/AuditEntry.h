/**
 * @file AuditEntry.h
 * @brief One record in a project's audit history.
 */
#pragma once

#include <string>

#include "DateTime.h"
#include "nlohmann/json.hpp"

/**
 * @brief Value type describing what happened, to what, and when.
 */
class AuditEntry {
public:
    std::string kind;           ///< EXECUTE, UNDO, REDO or REJECTED.
    std::string description;    ///< The command's `describe()` text.
    DateTime timestamp;         ///< When the entry was recorded.

    /**
     * @brief Constructs an audit entry.
     *
     * @param kind EXECUTE, UNDO, REDO or REJECTED.
     * @param description What happened.
     * @param timestamp When it happened.
     */
    AuditEntry(const std::string& kind, const std::string& description, const DateTime& timestamp);

    /**
     * @brief Formats the entry for display.
     *
     * @return `std::string` "[timestamp] KIND description".
     */
    std::string toString() const;

    /**
     * @brief Serialises the entry.
     *
     * @return `nlohmann::json` Object with kind, description and timestamp.
     */
    nlohmann::json toJson() const;
};
