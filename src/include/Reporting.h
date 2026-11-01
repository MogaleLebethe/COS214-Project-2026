#ifndef TIVIDY_REPORTING_H
#define TIVIDY_REPORTING_H

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

// Forward declaration
class WorkItemView;

/**
 * @brief Interface for querying project status, metrics, and audit histories.
 *
 * Provides read-only access to view reports, export project trees to JSON,
 * inspect available actionable work items, and review audit logs.
 */
class Reporting
{
public:
    /**
     * 
     * @brief Virtual destructor for the interface.
     */
    virtual ~Reporting() = default;

    /**
     * @brief Generates a human-readable status summary report for a project.
     *
     * @param projectId The unique identifier of the project.
     * @return `std::string` A formatted textual summary of the project state and completion progress.
     * @throws `NotFoundError` If the project ID cannot be found.
     */
    virtual std::string viewProjectStatus(std::string projectId) const = 0;

    /**
     * @brief Exports the entire project hierarchy tree into a serialisable JSON format.
     *
     * @param projectId The unique identifier of the project.
     * @return `nlohmann::json` A JSON object representing the full project structure.
     * @throws `NotFoundError` If the project ID cannot be found.
     */
    virtual nlohmann::json exportProjectToJson(std::string projectId) const = 0;

    /**
     * @brief Retrieves a list of work items that are currently ready for execution.
     *
     * @details Evaluates dependency readiness across the project tree and returns lightweight
     *          data transfer objects (`WorkItemView`) for active or pending work items.
     *
     * @param projectId The unique identifier of the project.
     * @return `std::vector<WorkItemView>` A collection of actionable work items.
     * @throws `NotFoundError` If the project ID cannot be found.
     */
    virtual std::vector<WorkItemView> listAvailableWork(std::string projectId) const = 0;

    /**
     * @brief Retrieves the append-only audit trail entries for a project.
     *
     * @param projectId The unique identifier of the project.
     * @return `std::vector<std::string>` A list of formatted audit log descriptions.
     * @throws `NotFoundError` If the project ID cannot be found.
     */
    virtual std::vector<std::string> getHistory(std::string projectId) const = 0;
};

#endif // TIVIDY_REPORTING_H