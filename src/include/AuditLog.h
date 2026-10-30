/**
 * @file AuditLog.h
 * @brief Append-only audit history of a project.
 */
#pragma once

#include <string>
#include <vector>

#include "AuditEntry.h"
#include "nlohmann/json.hpp"

class DateTime;

/**
 * @brief Records every action on a project.
 *
 * Kept separate from the undo stack: undo removes commands from the stack,
 * but the audit log never loses entries.
 */
class AuditLog {
private:
    std::vector<AuditEntry> entries;    ///< All entries, oldest first (stored by value).

public:
    /**
     * @brief Appends an entry. Existing entries are never removed or changed.
     *
     * @param kind EXECUTE, UNDO, REDO or REJECTED.
     * @param description What happened.
     * @param at When it happened.
     */
    void record(const std::string& kind, const std::string& description, const DateTime& at);

    /**
     * @brief Retrieves all entries.
     *
     * @return `const std::vector<AuditEntry>&` Read-only view of the entries, oldest first.
     */
    const std::vector<AuditEntry>& getEntries() const;

    /**
     * @brief Formats every entry for display.
     *
     * @return `std::vector<std::string>` One line per entry, oldest first.
     */
    std::vector<std::string> toStrings() const;

    /**
     * @brief Serialises the whole log.
     *
     * @return `nlohmann::json` Array of entry objects, oldest first.
     */
    nlohmann::json toJson() const;
};
