#include "Support.h"
#include "Command.h"
#include "Project.h"
#include "WorkflowError.h"
#include <ctime>
#include <iomanip>
#include <sstream>

// ==================== DateTime ====================

DateTime::DateTime(int y, int m, int d, int h, int min)
    : year(y), month(m), day(d), hour(h), min(min) {}

DateTime DateTime::now() {
    std::time_t t = std::time(nullptr);
    std::tm* nowTime = std::localtime(&t);
    return DateTime(
        nowTime->tm_year + 1900,
        nowTime->tm_mon + 1,
        nowTime->tm_mday,
        nowTime->tm_hour,
        nowTime->tm_min
    );
}

std::string DateTime::toString(const std::string& format) const {
    std::ostringstream oss;
    if (format == "yyyy/mm/dd") {
        oss << std::setfill('0') << std::setw(4) << year << "/"
            << std::setw(2) << month << "/"
            << std::setw(2) << day;
    } else {
        oss << std::setfill('0') << std::setw(4) << year << "-"
            << std::setw(2) << month << "-"
            << std::setw(2) << day << " "
            << std::setw(2) << hour << ":"
            << std::setw(2) << min;
    }
    return oss.str();
}

int DateTime::getYear() const { return year; }
int DateTime::getMonth() const { return month; }
int DateTime::getDay() const { return day; }
int DateTime::getHour() const { return hour; }
int DateTime::getMinute() const { return min; }

// ==================== Participant ====================

Participant::Participant(const std::string& id, const std::string& name, const std::string& role)
    : id(id), name(name), role(role) {}

std::string Participant::getId() const { return id; }
std::string Participant::getName() const { return name; }
std::string Participant::getRole() const { return role; }

// ==================== Comment ====================

Comment::Comment(const std::string& authorId, const std::string& text, const DateTime& timestamp)
    : authorId(authorId), text(text), timestamp(timestamp) {}

std::string Comment::getAuthorId() const { return authorId; }
std::string Comment::getText() const { return text; }
DateTime Comment::getTimestamp() const { return timestamp; }

// ==================== WorkflowDefinition ====================

WorkflowDefinition::WorkflowDefinition(const std::string& id, const std::string& name, 
                                     const std::string& industry, const std::string& description, 
                                     const nlohmann::json& templateJson)
    : id(id), name(name), industry(industry), description(description), templateJson(templateJson) {}

std::string WorkflowDefinition::getId() const { return id; }
std::string WorkflowDefinition::getName() const { return name; }
std::string WorkflowDefinition::getIndustry() const { return industry; }
std::string WorkflowDefinition::getDescription() const { return description; }
const nlohmann::json& WorkflowDefinition::getTemplate() const { return templateJson; }

// ==================== AuditEntry ====================

AuditEntry::AuditEntry(const std::string& timestamp, const std::string& actorId, const std::string& actionType, const std::string& details)
    : timestamp(timestamp), actorId(actorId), actionType(actionType), details(details) {}

std::string AuditEntry::getTimestamp() const { return timestamp; }
std::string AuditEntry::getActorId() const { return actorId; }
std::string AuditEntry::getActionType() const { return actionType; }
std::string AuditEntry::getDetails() const { return details; }

nlohmann::json AuditEntry::toJson() const {
    nlohmann::json j;
    j["timestamp"] = timestamp;
    j["actorId"] = actorId;
    j["actionType"] = actionType;
    j["details"] = details;
    return j;
}

// ==================== AuditLog ====================

AuditLog::AuditLog() = default;

AuditLog::~AuditLog() {
    clear();
}

void AuditLog::record(AuditEntry* entry) {
    if (entry) {
        entries.push_back(entry);
    }
}

const std::vector<AuditEntry*>& AuditLog::getEntries() const {
    return entries;
}

void AuditLog::clear() {
    for (AuditEntry* entry : entries) {
        delete entry;
    }
    entries.clear();
}

nlohmann::json AuditLog::toJson() const {
    nlohmann::json jArray = nlohmann::json::array();
    for (const AuditEntry* entry : entries) {
        if (entry) {
            jArray.push_back(entry->toJson());
        }
    }
    return jArray;
}

// ==================== CommandHistory ====================

CommandHistory::CommandHistory() = default;

CommandHistory::~CommandHistory() {
    clear();
}

void CommandHistory::executeCommand(Command* command) {
    if (!command) return;
    
    command->execute();
    undoStack.push_back(command);

    // Clear redo stack on new command execution
    for (Command* cmd : redoStack) {
        delete cmd;
    }
    redoStack.clear();
}

void CommandHistory::undo() {
    if (!canUndo()) {
        throw WorkflowError("Command history undo stack is empty.");
    }

    Command* command = undoStack.back();
    undoStack.pop_back();

    command->undo();
    redoStack.push_back(command);
}

void CommandHistory::redo() {
    if (!canRedo()) {
        throw WorkflowError("Command history redo stack is empty.");
    }

    Command* command = redoStack.back();
    redoStack.pop_back();

    command->execute();
    undoStack.push_back(command);
}

bool CommandHistory::canUndo() const {
    return !undoStack.empty();
}

bool CommandHistory::canRedo() const {
    return !redoStack.empty();
}

void CommandHistory::clear() {
    for (Command* cmd : undoStack) {
        delete cmd;
    }
    undoStack.clear();

    for (Command* cmd : redoStack) {
        delete cmd;
    }
    redoStack.clear();
}

// ==================== ProjectSession ====================

ProjectSession::ProjectSession(const std::string& sessionId, Project* project)
    : sessionId(sessionId), activeProject(project) {
    commandHistory = new CommandHistory();
    auditLog = new AuditLog();
}

ProjectSession::~ProjectSession() {
    delete commandHistory;
    delete auditLog;
}

std::string ProjectSession::getSessionId() const {
    return sessionId;
}

Project* ProjectSession::getActiveProject() const {
    return activeProject;
}

CommandHistory* ProjectSession::getCommandHistory() const {
    return commandHistory;
}

AuditLog* ProjectSession::getAuditLog() const {
    return auditLog;
}

void ProjectSession::executeCommand(Command* command) {
    if (!command) return;

    commandHistory->executeCommand(command);

    if (auditLog) {
        auditLog->record(new AuditEntry(
            DateTime::now().toString("yyyy-mm-dd HH:MM"), 
            sessionId, 
            "ExecuteCommand", 
            command->describe()
        ));
    }
}