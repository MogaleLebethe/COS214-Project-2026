#include "AuditLog.h"
#include "DateTime.h"

void AuditLog::record(const std::string& kind, const std::string& description, const DateTime& at) {
    entries.push_back(AuditEntry(kind, description, at));
}

const std::vector<AuditEntry>& AuditLog::getEntries() const {
    return entries;
}

std::vector<std::string> AuditLog::toStrings() const {
    std::vector<std::string> out;
    for (std::size_t i = 0; i < entries.size(); ++i)
        out.push_back(entries[i].toString());
    return out;
}

nlohmann::json AuditLog::toJson() const {
    nlohmann::json arr = nlohmann::json::array();
    for (std::size_t i = 0; i < entries.size(); ++i)
        arr.push_back(entries[i].toJson());
    return arr;
}
