#ifndef TIVIDY_SUPPORT_H
#define TIVIDY_SUPPORT_H

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

// Forward declarations
class Command;
class Project;
class AuditEntry;
class AuditLog;
class CommandHistory;

/**
 * @brief Represents date and time information for workflow deadlines, audit timestamps, and comments.
 */
class DateTime {
private:
    int year;  ///< Four-digit year.
    int month; ///< Month of the year (1-12).
    int day;   ///< Day of the month (1-31).
    int hour;  ///< Hour of the day (0-23).
    int min;   ///< Minute of the hour (0-59).

public:
    /**
     * @brief Constructs a `DateTime` instance.
     * 
     * @param y Four-digit year.
     * @param m Month (1-12).
     * @param d Day (1-31).
     * @param h Hour (0-23), default is 0.
     * @param min Minute (0-59), default is 0.
     */
    DateTime(int y, int m, int d, int h = 0, int min = 0);

    /**
     * @brief Virtual destructor.
     */
    virtual ~DateTime() = default;

    /**
     * @brief Retrieves the current system date and time.
     * 
     * @return `DateTime` Current date and time instance.
     */
    static DateTime now();

    /**
     * @brief Formats the date and time into a string representation.
     * 
     * @param format Format string (default is `"yyyy/mm/dd"`).
     * @return `std::string` Formatted date and time string.
     */
    std::string toString(const std::string& format = "yyyy/mm/dd") const;

    /**
     * @brief Retrieves the year.
     * 
     * @return `int` Year.
     */
    int getYear() const;

    /**
     * @brief Retrieves the month.
     * 
     * @return `int` Month.
     */
    int getMonth() const;

    /**
     * @brief Retrieves the day.
     * 
     * @return `int` Day.
     */
    int getDay() const;

    /**
     * @brief Retrieves the hour.
     * 
     * @return `int` Hour.
     */
    int getHour() const;

    /**
     * @brief Retrieves the minute.
     * 
     * @return `int` Minute.
     */
    int getMinute() const;
};

/**
 * @brief Represents a human participant or system actor involved in workflow components.
 */
class Participant {
private:
    std::string id;   ///< Unique identifier of the participant.
    std::string name; ///< Display name of the participant.
    std::string role; ///< Assigned role of the participant within the workflow.

public:
    /**
     * @brief Constructs a `Participant` instance.
     * 
     * @param id Unique participant ID.
     * @param name Participant display name.
     * @param role Participant role.
     */
    Participant(const std::string& id, const std::string& name, const std::string& role);

    /**
     * @brief Virtual destructor.
     */
    virtual ~Participant() = default;

    /**
     * @brief Retrieves the participant identifier.
     * 
     * @return `std::string` Participant ID.
     */
    std::string getId() const;

    /**
     * @brief Retrieves the participant name.
     * 
     * @return `std::string` Participant name.
     */
    std::string getName() const;

    /**
     * @brief Retrieves the participant role.
     * 
     * @return `std::string` Participant role.
     */
    std::string getRole() const;
};

/**
 * @brief Represents an annotated comment added to a workflow component by a participant.
 */
class Comment {
private:
    std::string authorId; ///< Identifier of the author who posted the comment.
    std::string text;     ///< Content of the comment.
    DateTime timestamp;   ///< Date and time when the comment was created.

public:
    /**
     * @brief Constructs a `Comment` instance.
     * 
     * @param authorId Identifier of the comment author.
     * @param text Comment text content.
     * @param timestamp Timestamp of creation.
     */
    Comment(const std::string& authorId, const std::string& text, const DateTime& timestamp);

    /**
     * @brief Virtual destructor.
     */
    virtual ~Comment() = default;

    /**
     * @brief Retrieves the author identifier.
     * 
     * @return `std::string` Author ID.
     */
    std::string getAuthorId() const;

    /**
     * @brief Retrieves the comment text.
     * 
     * @return `std::string` Comment text.
     */
    std::string getText() const;

    /**
     * @brief Retrieves the timestamp of the comment.
     * 
     * @return `DateTime` Creation timestamp.
     */
    DateTime getTimestamp() const;
};

/**
 * @brief Data Transfer Object (DTO) providing a summary view of a work item.
 */
struct WorkItemView {
    std::string id;       ///< Unique work item identifier.
    std::string name;     ///< Name of the work item.
    std::string status;   ///< Current state summary string.
    std::string executor; ///< Assigned executor ID or name.
    bool ready;           ///< Flag indicating whether dependencies are satisfied.
};

/**
 * @brief Encapsulates a reusable workflow definition template categorized by industry and ID.
 */
class WorkflowDefinition {
private:
    std::string id;             ///< Unique definition identifier.
    std::string name;           ///< Human-readable name of the workflow definition.
    std::string industry;       ///< Industry domain category.
    std::string description;    ///< Detailed description of the definition template.
    nlohmann::json templateJson;///< JSON template structure used to instantiate projects/tasks.

public:
    /**
     * @brief Constructs a `WorkflowDefinition` instance.
     * 
     * @param id Unique definition ID.
     * @param name Definition name.
     * @param industry Industry classification.
     * @param description Template description.
     * @param templateJson JSON template payload.
     */
    WorkflowDefinition(const std::string& id, const std::string& name, 
                       const std::string& industry, const std::string& description, 
                       const nlohmann::json& templateJson);

    /**
     * @brief Virtual destructor.
     */
    virtual ~WorkflowDefinition() = default;

    /**
     * @brief Retrieves the definition identifier.
     * 
     * @return `std::string` Definition ID.
     */
    std::string getId() const;

    /**
     * @brief Retrieves the definition name.
     * 
     * @return `std::string` Definition name.
     */
    std::string getName() const;

    /**
     * @brief Retrieves the industry classification.
     * 
     * @return `std::string` Industry category.
     */
    std::string getIndustry() const;

    /**
     * @brief Retrieves the template description.
     * 
     * @return `std::string` Description.
     */
    std::string getDescription() const;

    /**
     * @brief Retrieves the underlying JSON template.
     * 
     * @return `const nlohmann::json&` Const reference to the JSON template.
     */
    const nlohmann::json& getTemplate() const;
};

/**
 * @brief Represents a single immutable audit log record for tracking workflow and project actions.
 */
class AuditEntry {
private:
    std::string timestamp;  ///< ISO-8601 timestamp string of when the event occurred.
    std::string actorId;    ///< Identifier of the user or system component that performed the action.
    std::string actionType; ///< Category or name of the action performed.
    std::string details;    ///< Detailed description or JSON payload snapshot of the event.

public:
    /**
     * @brief Constructs an audit entry record.
     * 
     * @param timestamp ISO-8601 timestamp string.
     * @param actorId Identifier of the actor.
     * @param actionType Type of action performed.
     * @param details Descriptive detail string.
     */
    AuditEntry(const std::string& timestamp, const std::string& actorId, const std::string& actionType, const std::string& details);

    /**
     * @brief Virtual destructor.
     */
    virtual ~AuditEntry() = default;

    /**
     * @brief Retrieves the timestamp of the audit entry.
     * 
     * @return `std::string` Timestamp string.
     */
    std::string getTimestamp() const;

    /**
     * @brief Retrieves the identifier of the actor who performed the action.
     * 
     * @return `std::string` Actor ID string.
     */
    std::string getActorId() const;

    /**
     * @brief Retrieves the type of action performed.
     * 
     * @return `std::string` Action type string.
     */
    std::string getActionType() const;

    /**
     * @brief Retrieves the details of the audit entry.
     * 
     * @return `std::string` Detail string.
     */
    std::string getDetails() const;

    /**
     * @brief Serializes the audit entry into a JSON representation.
     * 
     * @return `nlohmann::json` JSON object representing the entry.
     */
    nlohmann::json toJson() const;
};

/**
 * @brief Manages a collection of immutable `AuditEntry` records for system auditing and compliance.
 */
class AuditLog {
private:
    std::vector<AuditEntry*> entries; ///< Collection of non-owning raw pointers to audit entries.

public:
    /**
     * @brief Constructs an empty audit log.
     */
    AuditLog();

    /**
     * @brief Destroys the audit log and cleans up dynamically allocated audit entries.
     */
    virtual ~AuditLog();

    /**
     * @brief Records a new audit entry in the log.
     * 
     * @param entry Raw pointer to the `AuditEntry` to record (log takes ownership).
     */
    void record(AuditEntry* entry);

    /**
     * @brief Retrieves all audit entries currently recorded.
     * 
     * @return `const std::vector<AuditEntry*>&` Const reference to the vector of audit entries.
     */
    const std::vector<AuditEntry*>& getEntries() const;

    /**
     * @brief Clears all records from the audit log.
     */
    void clear();

    /**
     * @brief Serializes the complete audit log history into a JSON array representation.
     * 
     * @return `nlohmann::json` JSON array of audit entries.
     */
    nlohmann::json toJson() const;
};

/**
 * @brief Manages execution and rollback history stacks supporting undo and redo operations.
 */
class CommandHistory {
private:
    std::vector<Command*> undoStack; ///< Stack of executed commands available for undo.
    std::vector<Command*> redoStack; ///< Stack of undone commands available for redo.

public:
    /**
     * @brief Constructs an empty command history container.
     */
    CommandHistory();

    /**
     * @brief Destroys the command history and cleans up any allocated command objects.
     */
    virtual ~CommandHistory();

    /**
     * @brief Executes a command, records it onto the undo stack, and clears the redo stack.
     * 
     * @param command Raw pointer to the `Command` to execute (history takes ownership).
     */
    void executeCommand(Command* command);

    /**
     * @brief Reverts the most recent command on the undo stack and moves it to the redo stack.
     * 
     * @throws `WorkflowError` If the undo stack is empty.
     */
    void undo();

    /**
     * @brief Re-executes the most recent command on the redo stack and moves it back to the undo stack.
     * 
     * @throws `WorkflowError` If the redo stack is empty.
     */
    void redo();

    /**
     * @brief Checks whether an undo operation can be performed.
     * 
     * @return `bool` `true` if the undo stack is non-empty, `false` otherwise.
     */
    bool canUndo() const;

    /**
     * @brief Checks whether a redo operation can be performed.
     * 
     * @return `bool` `true` if the redo stack is non-empty, `false` otherwise.
     */
    bool canRedo() const;

    /**
     * @brief Clears all commands from both undo and redo history stacks.
     */
    void clear();
};

/**
 * @brief Encapsulates the active state, command history, and audit trail for an open project session.
 */
class ProjectSession {
private:
    std::string sessionId;          ///< Unique identifier for the session.
    Project* activeProject;         ///< Non-owning raw pointer to the loaded project instance.
    CommandHistory* commandHistory; ///< Raw pointer to the command history manager for undo/redo.
    AuditLog* auditLog;             ///< Raw pointer to the session audit log tracker.

public:
    /**
     * @brief Constructs a project session container.
     * 
     * @param sessionId Unique session identifier string.
     * @param project Raw pointer to the active project instance.
     */
    ProjectSession(const std::string& sessionId, Project* project);

    /**
     * @brief Destroys the project session and cleans up owned history and audit resources.
     */
    virtual ~ProjectSession();

    /**
     * @brief Retrieves the unique session identifier.
     * 
     * @return `std::string` Session ID string.
     */
    std::string getSessionId() const;

    /**
     * @brief Retrieves the active project instance.
     * 
     * @return `Project*` Raw pointer to the active project.
     */
    Project* getActiveProject() const;

    /**
     * @brief Retrieves the command history manager.
     * 
     * @return `CommandHistory*` Raw pointer to command history.
     */
    CommandHistory* getCommandHistory() const;

    /**
     * @brief Retrieves the session audit log.
     * 
     * @return `AuditLog*` Raw pointer to the audit log.
     */
    AuditLog* getAuditLog() const;

    /**
     * @brief Executes a command within the context of this project session, recording it in history and audit logs.
     * 
     * @param command Raw pointer to the `Command` to execute.
     */
    void executeCommand(Command* command);
};

#endif // TIVIDY_SUPPORT_H