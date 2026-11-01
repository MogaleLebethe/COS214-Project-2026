#include "TaskState.h"
#include "Task.h"
#include "WorkflowError.h"

TaskState::~TaskState() = default;

// PendingState

TaskState* PendingState::instance() {
    static PendingState s;
    return &s;
}

void PendingState::start(Task* task) {
    if (!task->isReady()) {
        throw InvalidTransitionError("PendingState::start: Prerequisite not satisfied");
    }
    task->setState(InProgressState::instance());
}

void PendingState::complete(Task*) {
    throw InvalidTransitionError("PendingState::complete: Task has not been started yet");
}

void PendingState::block(Task* task) {
    task->setState(BlockedState::instance());
}

void PendingState::fail(Task*) {
    throw InvalidTransitionError("PendingState::fail: Not a valid event from Pending");
}

void PendingState::cancel(Task* task) {
    task->setState(CancelledState::instance());
}


void PendingState::skip(Task* task) {
    task->setState(SkippedState::instance());
}

std::string PendingState::toString()const {
    return "Pending";
}

bool PendingState::isSatisfied() const {
    return false;
}

//InProgressState

TaskState* InProgressState::instance() {
    static InProgressState s;
    return &s;
}

void InProgressState::start(Task*) {
    throw InvalidTransitionError("InProgressState::start: Task is already in progress");
}

void InProgressState::complete(Task* task) {
    task->setState(CompletedState::instance());
}

void InProgressState::block(Task* task) {
    task->setState(BlockedState::instance());
}

void InProgressState::fail(Task* task) {
    task->setState(FailedState::instance());
}
void InProgressState::cancel(Task* task) {
    task->setState(CancelledState::instance());
}

void InProgressState::skip(Task*) {
    throw InvalidTransitionError("InProgressState::skip: Not a valid event from In Progress");
}

std::string InProgressState::toString() const {
    return "In Progress";
}

bool InProgressState::isSatisfied() const{
    return false;
}

//BlockedState

TaskState* BlockedState::instance() {
    static BlockedState s;
    return &s;
}
void BlockedState::start(Task* task) {
    task->setState(InProgressState::instance());
}

void BlockedState::complete(Task*) {
    throw InvalidTransitionError("BlockedState::complete: Task is blocked until resumed");
}

void BlockedState::block(Task*) {
    throw InvalidTransitionError("BlockedState::block: Task is already blocked");
}

void BlockedState::fail(Task* task) {
    task->setState(FailedState::instance());
}

void BlockedState::cancel(Task* task) {
    task->setState( CancelledState::instance());
}

void BlockedState::skip(Task*) {
    throw InvalidTransitionError("BlockedState::skip: Not a valid event from Blocked");
}
std::string BlockedState::toString() const {
    return "Blocked";
}

bool BlockedState::isSatisfied() const {
    return false;
}

//CompletedState

TaskState* CompletedState::instance() {
    static CompletedState s;
    return &s;
}

void CompletedState::start(Task* task) {
    task->setState(InProgressState::instance());
}
void CompletedState::complete(Task*) {
    throw InvalidTransitionError("CompletedState::complete: Task is already completed");
}

void CompletedState::block(Task*) {
    throw InvalidTransitionError("CompletedState::block: A completed task cant be blocked");
}
void CompletedState::fail(Task*) {
    throw InvalidTransitionError("CompletedState::fail: A completed task cant fail");
}

void CompletedState::cancel(Task*) {
    throw InvalidTransitionError("CompletedState::cancel: A completed task cant be cancelled");
}

void CompletedState::skip(Task*) {
    throw InvalidTransitionError("CompletedState::skip: A completed task cant be skipped");
}

std::string CompletedState::toString()const {
    return "Complete";
}

bool CompletedState::isSatisfied() const {
    return true;
}

// ==================== FailedState ====================

TaskState* FailedState::instance() {
    static FailedState s;
    return &s;
}

void FailedState::start(Task* task) {
    task->setState(InProgressState::instance());
}

void FailedState::complete(Task*) {
    throw InvalidTransitionError("FailedState::complete: Failed task cant be completed");
}

void FailedState::block(Task*) {
    throw InvalidTransitionError("FailedState::block: Not a valid event from Failed");
}

void FailedState::fail(Task*) {
    throw InvalidTransitionError("FailedState::fail: Task has already failed");
}
void FailedState::cancel(Task* task) {
    task->setState(CancelledState::instance());
}

void FailedState::skip(Task*) {
    throw InvalidTransitionError("FailedState::skip: Not a valid event from Failed");
}

std::string FailedState::toString() const {
    return "Failed";

}

bool FailedState::isSatisfied()const {
    return false;
}

//CancelledState

TaskState* CancelledState::instance() {
    static CancelledState s;
    return &s;
}

void CancelledState::start(Task*) {
    throw InvalidTransitionError("CancelledState::start: Cancelled is a terminal state");
}

void CancelledState::complete(Task*) {
    throw InvalidTransitionError("CancelledState::complete: Cancelled is a terminal state");
}

void CancelledState::block(Task*) {
    throw InvalidTransitionError("CancelledState::block: Cancelled is a terminal state");
}

void CancelledState::fail(Task*) {

    throw InvalidTransitionError("CancelledState::fail: Cancelled is a terminal state");
}

void CancelledState::cancel(Task*) {
    throw InvalidTransitionError("CancelledState::cancel: Cancelled is a terminal state");
}

void CancelledState::skip(Task*) {
    throw InvalidTransitionError("CancelledState::skip: Cancelled is a terminal state");
}

std::string CancelledState::toString()const {
    return "Cancelled";
}

bool CancelledState::isSatisfied() const {
    return false;
}

//SkippedState

TaskState* SkippedState::instance() {
    static SkippedState s;
    return &s;
}

void SkippedState::start(Task*) {
    throw InvalidTransitionError("SkippedState::start: Skipped is a terminal state");
}

void SkippedState::complete(Task*) {
    throw InvalidTransitionError("SkippedState::complete: Skipped is a terminal state");
}

void SkippedState::block(Task*) {
    throw InvalidTransitionError("SkippedState::block: Skipped is a terminal state");
}

void SkippedState::fail(Task*) {
    throw InvalidTransitionError("SkippedState::fail: Skipped is a terminal state");
}

void SkippedState::cancel(Task*) {

    throw InvalidTransitionError("SkippedState::cancel: Skipped is a terminal  state");
}

void SkippedState::skip(Task*) {
    throw InvalidTransitionError("SkippedState::skip: Skipped is a terminal state");
}

std::string SkippedState::toString() const {
    return "Skipped";
}

bool SkippedState::isSatisfied() const {
    return true;
}