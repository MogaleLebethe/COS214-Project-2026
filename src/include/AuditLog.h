#pragma once

#include "AuditEntry.h"
#include "nlohmann/json.hpp"
#include <string>
#include <vector>

class DateTime;

// Append-only; undo never removes entries.
class AuditLog {
private:
    std::vector<AuditEntry> entries;

public:
    void record(const std::string& kind, const std::string& description, const DateTime& at);

    const std::vector<AuditEntry>& getEntries() const;
    std::vector<std::string> toStrings() const;
    nlohmann::json toJson() const;
};
