#ifndef TIVIDY_WORKFLOW_COMPONENT_H
#define TIVIDY_WORKFLOW_COMPONENT_H

#include <map>
#include <string>
#include <vector>
#include "nlohmann/json.hpp"

// Forward declarations
class TaskBuilder;
class ProjectBuilder;
class DecisionTaskBuilder;
class Project;
class TaskMemento;
class Participant;
class DateTime;
class Comment;
class WorkItemView;

/**
 * @brief Abstract base class representing a component in the workflow composite tree hierarchy.
 * 
 * Manages identity, descriptions, executors, deadlines, metadata fields, comments, participants,
 * and observer pattern dependency/dependent link bindings.
 */
class WorkflowComponent {
public:
    virtual ~WorkflowComponent();

    /**
     * @brief Retrieves the unique identifier of the component.
     */
    std::string getId() const;

    /**
     * @brief Retrieves the name of the component.
     */
    std::string getName() const;

    /**
     * @brief Sets the name of the component.
     */
    void setName(const std::string& name);

    /**
     * @brief Retrieves the description of the component.
     */
    std::string getDescription() const;

    /**
     * @brief Sets the description of the component.
     */
    void setDescription(const std::string& desc);

    /**
     * @brief Assigns a participant executor to the component.
     */
    void setExecutor(const Participant& p);

    /**
     * @brief Retrieves the executor identifier string.
     */
    std::string getExecutorId() const;

    /**
     * @brief Adds a participant to the component.
     */
    std::string addParticipant(Participant p);

    /**
     * @brief Sets the deadline for the component.
     */
    void setDeadline(const DateTime& d);

    /**
     * @brief Adds a comment to the component.
     */
    void addComment(Comment comment);

    /**
     * @brief Sets a custom key-value metadata field.
     */
    void setCustomField(const std::string& key, const std::string& value);

    /**
     * @brief Retrieves a custom metadata field value by its key.
     */
    std::string getCustomField(const std::string& key) const;

    /**
     * @brief Adds a prerequisite dependency component.
     */
    bool addDependency(WorkflowComponent* prerequisite);

    /**
     * @brief Removes a prerequisite dependency component.
     */
    void removeDependency(WorkflowComponent* prerequisite);

    /**
     * @brief Adds a dependent successor component.
     */
    void addDependent(WorkflowComponent* component);

    /**
     * @brief Removes a dependent successor component.
     */
    void removeDependent(WorkflowComponent* component);

    /**
     * @brief Retrieves a const reference to the vector of prerequisite dependency component pointers.
     */
    const std::vector<WorkflowComponent*>& getDependencies() const;

    /**
     * @brief Notifies all registered dependents of a state or status change.
     */
    void notifyDependents();

    /**
     * @brief Observer update callback triggered when a dependency changes state.
     */
    virtual void update(WorkflowComponent* changedComponent);

    /**
     * @brief Checks whether the component is ready for execution.
     */
    virtual bool isReady() const;

    /**
     * @brief Evaluates whether prerequisite dependencies are satisfied.
     */
    virtual void evaluateDependencies() = 0;

    /**
     * @brief Checks whether the component has completed its execution lifecycle.
     */
    virtual bool isCompleted() const = 0;

    /**
     * @brief Generates a textual summary string of the component's status.
     */
    virtual std::string getStatusSummary() const = 0;

    /**
     * @brief Collects actionable work items into a status data transfer view list.
     */
    virtual void collectWork(std::vector<WorkItemView>& out) const = 0;

    /**
     * @brief Serialises the component into a structured JSON representation.
     */
    virtual nlohmann::json toJson() const = 0;

    /**
     * @brief Creates a deep-copy prototype clone of the component.
     */
    virtual WorkflowComponent* clone() const = 0;

protected:
    WorkflowComponent();
    WorkflowComponent(const WorkflowComponent& other);

    /**
     * @brief Recomputes the readiness status based on current dependencies.
     */
    void recomputeReadiness();

    /**
     * @brief Unlinks all dependency and dependent connection references.
     */
    virtual void unlinkAll();

    std::string id;                                   ///< Unique string identifier for the component.
    std::string name;                                 ///< Name of the workflow component.
    std::string description;                          ///< Description of the workflow component.
    std::string executorId;                             ///< Assigned executor participant object.
    DateTime deadline;                                ///< Deadline representation object.
    std::map<std::string, std::string> customFields;  ///< Custom metadata key-value pairs.
    std::vector<Comment> comments;                    ///< Collection of comments attached.
    std::vector<Participant> participants;            ///< Collection of participants involved.
    std::vector<WorkflowComponent*> dependencies;     ///< Prerequisite dependency components.
    std::vector<WorkflowComponent*> dependents;       ///< Successor dependent components.
    bool ready = true;                                       ///< Flag indicating execution readiness.

    friend class TaskBuilder;
    friend class ProjectBuilder;
    friend class DecisionTaskBuilder;
    friend class Project;
    friend class TaskMemento;

private:
    WorkflowComponent& operator=(const WorkflowComponent&) = delete;
};

#endif // TIVIDY_WORKFLOW_COMPONENT_H