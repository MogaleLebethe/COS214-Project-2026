#pragma once

#include "DateTime.h"
#include "nlohmann/json.hpp"
#include <string>

class AuditEntry {
public:
    std::string kind;           // EXECUTE, UNDO, REDO, REJECTED
    std::string description;
    DateTime timestamp;

    AuditEntry(const std::string& kind, const std::string& description, const DateTime& timestamp);

    std::string toString() const;
    nlohmann::json toJson() const;
};
