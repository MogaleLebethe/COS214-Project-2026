#ifndef TIVIDY_WORKFLOW_FACTORY_H
#define TIVIDY_WORKFLOW_FACTORY_H

#include <string>
#include <map>
#include <nlohmann/json.hpp>

// Forward declarations
class WorkflowComponent;
class Task;
class DecisionTask;
class Project;
class Participant;
class DateTime;
class RoutingStrategy;
class RouterRegistry;

/**
 * @brief Abstract base builder interface for creating and assembling workflow components using raw pointers.
 */
class ComponentBuilder {
public:
    virtual ~ComponentBuilder() = default;

    /**
     * @brief Resets the builder state to prepare for building a new component.
     */
    virtual void reset() = 0;

    /**
     * @brief Sets basic identification information for the component.
     */
    virtual void setBasicInfo(const std::string& id, const std::string& name) = 0;

    /**
     * @brief Sets the description of the component.
     */
    virtual void setDescription(const std::string& desc) = 0;

    /**
     * @brief Assigns an executor participant to the component.
     */
    virtual void setExecutor(const Participant& executor) = 0;

    /**
     * @brief Sets the deadline for the component.
     */
    virtual void setDeadline(const DateTime& deadline) = 0;

    /**
     * @brief Adds a prerequisite dependency component.
     */
    virtual void addDependency(WorkflowComponent* component) = 0;

    /**
     * @brief Adds a custom key-value metadata field.
     */
    virtual void addCustomField(const std::string& key, const std::string& value) = 0;

    /**
     * @brief Adds a child work item (for composite containers like projects).
     */
    virtual void addWorkItem(WorkflowComponent* item) = 0;

    /**
     * @brief Finalizes and returns the built workflow component (caller takes ownership).
     */
    virtual WorkflowComponent* build() = 0;
};

/**
 * @brief Builder implementation specialized for constructing standard `Task` instances.
 */
class TaskBuilder : public ComponentBuilder {
protected:
    Task* product; ///< Raw pointer to the task instance under construction.

public:
    TaskBuilder();
    virtual ~TaskBuilder();

    void reset() override;
    void setBasicInfo(const std::string& id, const std::string& name) override;
    void setDescription(const std::string& desc) override;
    void setExecutor(const Participant& executor) override;
    void setDeadline(const DateTime& deadline) override;
    void addDependency(WorkflowComponent* component) override;
    void addCustomField(const std::string& key, const std::string& value) override;
    void addWorkItem(WorkflowComponent* item) override;
    WorkflowComponent* build() override;

    /**
     * @brief Retrieves the concrete typed task result.
     */
    Task* getResult();
};

/**
 * @brief Builder implementation specialized for constructing `DecisionTask` instances with routing branches.
 */
class DecisionTaskBuilder : public TaskBuilder {
private:
    DecisionTask* decisionProduct; ///< Raw pointer to the decision task instance under construction.

public:
    DecisionTaskBuilder();
    virtual ~DecisionTaskBuilder() = default;

    void reset() override;
    WorkflowComponent* build() override;

    /**
     * @brief Sets the evaluation routing strategy for the decision task.
     */
    void setRouter(RoutingStrategy* r);

    /**
     * @brief Adds a branch target mapping for a decision label.
     */
    void addBranchTarget(const std::string& label, Task* target);

    /**
     * @brief Retrieves the concrete typed decision task result.
     */
    DecisionTask* getDecisionResult();
};

/**
 * @brief Builder implementation specialized for constructing `Project` containers.
 */
class ProjectBuilder : public ComponentBuilder {
private:
    Project* product; ///< Raw pointer to the project instance under construction.

public:
    ProjectBuilder();
    virtual ~ProjectBuilder();

    void reset() override;
    void setBasicInfo(const std::string& id, const std::string& name) override;
    void setDescription(const std::string& desc) override;
    void setExecutor(const Participant& executor) override;
    void setDeadline(const DateTime& deadline) override;
    void addDependency(WorkflowComponent* component) override;
    void addCustomField(const std::string& key, const std::string& value) override;
    void addWorkItem(WorkflowComponent* item) override;
    WorkflowComponent* build() override;

    /**
     * @brief Retrieves the concrete typed project result.
     */
    Project* getResult();
};

/**
 * @brief Director that orchestrates builders using type registries to hydrate components from templates.
 */
class Director {
private:
    std::map<std::string, ComponentBuilder*> builders; ///< Map of component type keys to their respective builders.
    const RouterRegistry* routerRegistry;              ///< Non-owning pointer to router registry for decision tasks.

public:
    /**
     * @brief Constructs a Director with an optional router registry reference.
     */
    explicit Director(const RouterRegistry* routerRegistry = nullptr);

    virtual ~Director() = default;

    /**
     * @brief Registers a component builder against a type string key.
     */
    void registerBuilder(const std::string& type, ComponentBuilder* b);

    /**
     * @brief Creates a new standard task instance.
     */
    Task* createNewTask(const std::string& name, const std::string& desc = "");

    /**
     * @brief Creates a new project instance.
     */
    Project* createNewProject(const std::string& name, const std::string& desc = "");

    /**
     * @brief Hydrates and creates a workflow component tree dynamically from a JSON template definition.
     */
    WorkflowComponent* createFromTemplate(const nlohmann::json& templateJson);
};

/**
 * @brief Factory facade managing builders and directors for clean workflow instantiation.
 */
class WorkflowFactory {
private:
    TaskBuilder taskBuilder;                 ///< Task builder instance.
    DecisionTaskBuilder decisionBuilder;     ///< Decision task builder instance.
    ProjectBuilder projectBuilder;           ///< Project builder instance.
    Director director;                       ///< Director orchestrator.

public:
    /**
     * @brief Constructs a WorkflowFactory and initializes builder registrations with the director.
     */
    explicit WorkflowFactory(const RouterRegistry* routerRegistry = nullptr);

    virtual ~WorkflowFactory() = default;

    /**
     * @brief Creates a new root project instance.
     */
    Project* newProject(const std::string& name, const std::string& desc = "");

    /**
     * @brief Creates a new task instance.
     */
    Task* newTask(const std::string& name, const std::string& desc = "");

    /**
     * @brief Creates a workflow component or project from a JSON template.
     */
    WorkflowComponent* fromTemplate(const nlohmann::json& templateJson);
};

#endif // TIVIDY_WORKFLOW_FACTORY_H