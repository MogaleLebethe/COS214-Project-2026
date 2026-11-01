#ifndef TIVIDY_PROJECT_H
#define TIVIDY_PROJECT_H

#include "WorkflowComponent.h"
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

// Forward declarations
class WorkItemView;
class ProjectBuilder;
class Participant;

/**
 * @brief Represents a container project node in the workflow component composite hierarchy.
 * 
 * A project acts as a container holding a list of `WorkflowComponent` work items. 
 * Other projects can also be nested inside a project. The project owns its children 
 * (`workItems`) and releases them upon destruction or transfers ownership back to 
 * callers upon removal.
 */
class Project : public WorkflowComponent {
public:
    /**
     * @brief Constructs a new Project with a generated unique identifier and default state.
     */
    Project();

    /**
     * @brief Virtual destructor for the project container, releasing owned child work items.
     */
    virtual ~Project();

    /**
     * @brief Generates a unique string identifier for a new project instance.
     */
    static std::string generateId();

    /**
     * @brief Adds a child work item component to the project container, taking ownership.
     */
    void addWorkItem(WorkflowComponent* item);

    /**
     * @brief Removes a child work item component from the project list and returns it, transferring ownership back to the caller.
     */
    WorkflowComponent* removeWorkItem(WorkflowComponent* item);

    /**
     * @brief Retrieves a const reference to the vector of borrowed pointers to the work items.
     */
    const std::vector<WorkflowComponent*>& getWorkItems() const;

    /**
     * @brief Finds and retrieves a workflow component by its unique identifier within the project tree.
     */
    WorkflowComponent* findById(const std::string& id) const;

    /**
     * @brief Evaluates the dependencies of all work items contained within the project.
     */
    void evaluateDependencies() override;

    /**
     * @brief Checks whether all child work items in the project have completed.
     */
    bool isCompleted() const override;

    /**
     * @brief Generates a textual summary string of the project status.
     */
    std::string getStatusSummary() const override;

    /**
     * @brief Collects available actionable work items from all child subtrees.
     */
    void collectWork(std::vector<WorkItemView>& out) const override;

    /**
     * @brief Serializes the project and its child work items into a structured JSON representation.
     */
    nlohmann::json toJson() const override;

    /**
     * @brief Creates a deep-copy prototype clone of the project container and its work items.
     */
    WorkflowComponent* clone() const override;

protected:
    /**
     * @brief Copy constructor for prototype-based project cloning.
     */
    Project(const Project& other);

    /**
     * @brief Unlinks all dependency, dependent, and child work item references.
     */
    void unlinkAll() override;

private:
    /**
     * @brief Checks recursively whether the project contains a specified component.
     */
    bool contains(const WorkflowComponent* component) const;

    std::vector<WorkflowComponent*> workItems; ///< The list of work items owned by the project.
    static int counter;                        ///< Static counter used for generating sequential project IDs.

    friend class ProjectBuilder;
};

#endif // TIVIDY_PROJECT_H