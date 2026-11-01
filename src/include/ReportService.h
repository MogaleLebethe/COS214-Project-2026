#ifndef TIVIDY_REPORT_SERVICE_H
#define TIVIDY_REPORT_SERVICE_H

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

// Forward declarations
class Project;
class WorkItemView;

/**
 * @brief Service responsible for generating status summaries, metrics, and exports from project graphs.
 * 
 * Decouples reporting presentation logic from core domain models by traversing 
 * project trees to build text summaries, JSON exports, and actionable work lists.
 */
class ReportService {
public:
    /**
     * @brief Virtual destructor for the service.
     */
    virtual ~ReportService() = default;

    /**
     * @brief Formats a comprehensive status summary string for a project.
     * 
     * @param p Const reference to the root project object.
     * @return `std::string` A formatted text block summarising project status and task states.
     */
    std::string statusOf(const Project& p) const;

    /**
     * @brief Serialises a project hierarchy into a structured JSON representation.
     * 
     * @param p Const reference to the root project object.
     * @return `nlohmann::json` The serialised JSON tree.
     */
    nlohmann::json exportJson(const Project& p) const;

    /**
     * @brief Collects all actionable work items ready for execution across a project tree.
     * 
     * @param p Const reference to the root project object.
     * @return `std::vector<WorkItemView>` A vector containing summaries of ready work items.
     */
    std::vector<WorkItemView> availableWork(const Project& p) const;
};

#endif // TIVIDY_REPORT_SERVICE_H