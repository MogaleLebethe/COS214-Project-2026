#include "ReportService.h"
#include "Support.h"
#include "Project.h"

std::string ReportService::statusOf(const Project &p) const { return p.getStatusSummary(); }

nlohmann::json ReportService::exportJson(const Project &p) const { return p.toJson(); }

std::vector<WorkItemView> ReportService::availableWork(const Project &p) const
{
    std::vector<WorkItemView> views;
    p.collectWork(views);
    return views;
}
