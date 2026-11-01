#include "WorkflowFactory.h"
#include "Project.h"
#include "Task.h"
#include "DecisionTask.h"
#include "WorkflowComponent.h"
#include "Support.h"
#include "WorkflowError.h"
#include "RouterRegistry.h"
#include "RoutingStrategy.h"
#include <stdexcept>
#include <algorithm>

// ==========================================
// TaskBuilder Implementation
// ==========================================

TaskBuilder::TaskBuilder() : product(nullptr) {
    reset();
}

TaskBuilder::~TaskBuilder() {
    delete product;
}

void TaskBuilder::reset() {
    delete product;
    product = nullptr;
    product = new Task();
}

void TaskBuilder::setBasicInfo(const std::string& id, const std::string& name) {
    if (!id.empty()) {
        product->id = id;
    }
    product->name = name;
}

void TaskBuilder::setDescription(const std::string& desc) {
    product->description = desc;
}

void TaskBuilder::setExecutor(const Participant& executor) {
    product->executor = executor;
}

void TaskBuilder::setDeadline(const DateTime& deadline) {
    product->deadline = deadline;
}

void TaskBuilder::addDependency(WorkflowComponent* component) {
    product->addDependency(component);
}

void TaskBuilder::addCustomField(const std::string& key, const std::string& value) {
    product->setCustomField(key, value);
}

void TaskBuilder::addWorkItem(WorkflowComponent*) {
    throw std::logic_error("TaskBuilder::addWorkItem: Tasks cannot contain nested work items");
}

WorkflowComponent* TaskBuilder::build() {
    return getResult();
}

Task* TaskBuilder::getResult() {
    if (product->getName().empty()) {
        throw WorkflowError("TaskBuilder::validate: Task name is mandatory");
    }
    Task* finished = product;
    product = nullptr;
    try {
        reset();
    } catch (...) {
        delete finished;
        throw;
    }
    return finished;
}


// ==========================================
// DecisionTaskBuilder Implementation
// ==========================================

DecisionTaskBuilder::DecisionTaskBuilder() : decisionProduct(nullptr) {
    reset();
}

void DecisionTaskBuilder::reset() {
    delete product;
    product = nullptr;
    decisionProduct = new DecisionTask();
    product = decisionProduct; // product pointer from TaskBuilder base points to DecisionTask
}

WorkflowComponent* DecisionTaskBuilder::build() {
    return getDecisionResult();
}

void DecisionTaskBuilder::setRouter(RoutingStrategy* r) {
    if (decisionProduct) {
        decisionProduct->setRouter(r);
    }
}

void DecisionTaskBuilder::addBranchTarget(const std::string& label, Task* target) {
    if (decisionProduct) {
        decisionProduct->addBranchTarget(label, target);
    }
}

DecisionTask* DecisionTaskBuilder::getDecisionResult() {
    if (decisionProduct->getName().empty()) {
        throw WorkflowError("DecisionTaskBuilder::validate: DecisionTask name is mandatory");
    }
    DecisionTask* finished = decisionProduct;
    product = nullptr;
    decisionProduct = nullptr;
    try {
        reset();
    } catch (...) {
        delete finished;
        throw;
    }
    return finished;
}


// ==========================================
// ProjectBuilder Implementation
// ==========================================

ProjectBuilder::ProjectBuilder() : product(nullptr) {
    reset();
}

ProjectBuilder::~ProjectBuilder() {
    delete product;
}

void ProjectBuilder::reset() {
    delete product;
    product = nullptr;
    product = new Project();
}

void ProjectBuilder::setBasicInfo(const std::string& id, const std::string& name) {
    if (!id.empty()) {
        product->id = id;
    }
    product->name = name;
}

void ProjectBuilder::setDescription(const std::string& desc) {
    product->description = desc;
}

void ProjectBuilder::setExecutor(const Participant& executor) {
    product->executor = executor;
}

void ProjectBuilder::setDeadline(const DateTime& deadline) {
    product->deadline = deadline;
}

void ProjectBuilder::addDependency(WorkflowComponent* component) {
    product->addDependency(component);
}

void ProjectBuilder::addCustomField(const std::string& key, const std::string& value) {
    product->setCustomField(key, value);
}

void ProjectBuilder::addWorkItem(WorkflowComponent* item) {
    product->addWorkItem(item);
}

WorkflowComponent* ProjectBuilder::build() {
    return getResult();
}

Project* ProjectBuilder::getResult() {
    if (product->getName().empty()) {
        throw WorkflowError("ProjectBuilder::validate: Project name is mandatory");
    }
    Project* finished = product;
    product = nullptr;
    try {
        reset();
    } catch (...) {
        delete finished;
        throw;
    }
    return finished;
}


// ==========================================
// Director Implementation
// ==========================================

Director::Director(const RouterRegistry* registry) : routerRegistry(registry) {}

void Director::registerBuilder(const std::string& type, ComponentBuilder* b) {
    if (b == nullptr) {
        throw std::invalid_argument("Director::registerBuilder: builder must not be null");
    }
    builders[type] = b;
}

Task* Director::createNewTask(const std::string& name, const std::string& desc) {
    std::map<std::string, ComponentBuilder*>::iterator found = builders.find("Task");
    if (found == builders.end() || found->second == nullptr) {
        throw WorkflowError("Director::createNewTask: No builder registered for \"Task\"");
    }
    ComponentBuilder* b = found->second;
    b->reset();
    b->setBasicInfo("", name);
    b->setDescription(desc);
    
    WorkflowComponent* comp = b->build();
    Task* task = dynamic_cast<Task*>(comp);
    if (!task) {
        delete comp;
        throw WorkflowError("Director::createNewTask: Builder for \"Task\" did not produce a valid Task");
    }
    return task;
}

Project* Director::createNewProject(const std::string& name, const std::string& desc) {
    std::map<std::string, ComponentBuilder*>::iterator found = builders.find("Project");
    if (found == builders.end() || found->second == nullptr) {
        throw WorkflowError("Director::createNewProject: No builder registered for \"Project\"");
    }
    ComponentBuilder* b = found->second;
    b->reset();
    b->setBasicInfo("", name);
    b->setDescription(desc);
    
    WorkflowComponent* comp = b->build();
    Project* proj = dynamic_cast<Project*>(comp);
    if (!proj) {
        delete comp;
        throw WorkflowError("Director::createNewProject: Builder for \"Project\" did not produce a valid Project");
    }
    return proj;
}

WorkflowComponent* Director::createFromTemplate(const nlohmann::json& templateJson) {
    if (!templateJson.contains("type")) {
        throw WorkflowError("Director::createFromTemplate: JSON template missing \"type\" field");
    }
    std::string type = templateJson["type"];
    std::map<std::string, ComponentBuilder*>::iterator found = builders.find(type);
    if (found == builders.end() || found->second == nullptr) {
        throw NotFoundError("Director::createFromTemplate: No builder registered for type: " + type);
    }
    
    ComponentBuilder* b = found->second;
    b->reset();

    std::string id = templateJson.value("id", "");
    std::string name = templateJson.value("name", "");
    std::string desc = templateJson.value("description", "");
    b->setBasicInfo(id, name);
    b->setDescription(desc);

    if (templateJson.contains("executor") && templateJson["executor"].is_object()) {
        nlohmann::json exJson = templateJson["executor"];
        std::string exId = exJson.value("id", "");
        std::string exName = exJson.value("name", "");
        std::string exRole = exJson.value("role", "");
        b->setExecutor(Participant(exId, exName, exRole));
    }

    if (templateJson.contains("customFields") && templateJson["customFields"].is_object()) {
        for (nlohmann::json::const_iterator it = templateJson["customFields"].begin();
             it != templateJson["customFields"].end(); ++it) {
            b->addCustomField(it.key(), it.value().get<std::string>());
        }
    }

    // Handle DecisionTask specific configurations if router info is present
    if (type == "Decision" || type == "DecisionTask") {
        DecisionTaskBuilder* decisionBuilderPtr = dynamic_cast<DecisionTaskBuilder*>(b);
        if (decisionBuilderPtr && templateJson.contains("router")) {
            std::string routerName = templateJson["router"].get<std::string>();
            if (routerRegistry) {
                RoutingStrategy* strat = routerRegistry->get(routerName);
                if (strat) {
                    decisionBuilderPtr->setRouter(strat);
                }
            }
        }
    }

    // Handle nested work items for projects
    if (templateJson.contains("workItems") && templateJson["workItems"].is_array()) {
        const nlohmann::json& items = templateJson["workItems"];
        for (std::size_t i = 0; i < items.size(); ++i) {
            WorkflowComponent* child = createFromTemplate(items[i]);
            try {
                b->addWorkItem(child);
            } catch (...) {
                delete child;
                throw;
            }
        }
    }

    return b->build();
}


// ==========================================
// WorkflowFactory Implementation
// ==========================================

WorkflowFactory::WorkflowFactory(const RouterRegistry* routerRegistry)
    : director(routerRegistry) {
    director.registerBuilder("Task", &taskBuilder);
    director.registerBuilder("Decision", &decisionBuilder);
    director.registerBuilder("DecisionTask", &decisionBuilder);
    director.registerBuilder("Project", &projectBuilder);
}

Project* WorkflowFactory::newProject(const std::string& name, const std::string& desc) {
    return director.createNewProject(name, desc);
}

Task* WorkflowFactory::newTask(const std::string& name, const std::string& desc) {
    return director.createNewTask(name, desc);
}

WorkflowComponent* WorkflowFactory::fromTemplate(const nlohmann::json& templateJson) {
    return director.createFromTemplate(templateJson);
}