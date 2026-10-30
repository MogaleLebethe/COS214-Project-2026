#include "AuditEntry.h"

static const char* const TIMESTAMP_FORMAT = "%Y-%m-%d %H:%M";

AuditEntry::AuditEntry(const std::string& kind, const std::string& description, const DateTime& timestamp)
    : kind(kind), description(description), timestamp(timestamp) {}

std::string AuditEntry::toString() const {
    return "[" + timestamp.toString(TIMESTAMP_FORMAT) + "] " + kind + " " + description;
}

nlohmann::json AuditEntry::toJson() const {
    nlohmann::json j;
    j["kind"] = kind;
    j["description"] = description;
    j["timestamp"] = timestamp.toString(TIMESTAMP_FORMAT);
    return j;
}
