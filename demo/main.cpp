/**
 * @file main.cpp
 * @brief Demo of the workflow engine with two scenarios: a hospital ER admission and an ad agency campaign.
 *
 * Usage:
 *   ./demo            interactive menu (default)
 *   ./demo hospital   scripted walkthrough of the hospital scenario
 *   ./demo agency     scripted walkthrough of the ad agency scenario
 *   ./demo all        both scripted walkthroughs
 *
 * Patterns on show: Facade, Builder/Director (templates), Composite (Project/Task), Observer (dependencies),
 * State (task lifecycle), Command (undo/redo), Memento (task edits), Strategy (decision routing),
 * Prototype (clone project), Registry (actions, routers, definitions).
 */
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "ActionRegistry.h"
#include "DecisionTask.h"
#include "DefinitionRepository.h"
#include "Project.h"
#include "ProjectRegistry.h"
#include "ReportService.h"
#include "RouterRegistry.h"
#include "RoutingStrategy.h"
#include "Support.h"
#include "Task.h"
#include "TaskState.h"
#include "WorkflowEngineFacade.h"
#include "WorkflowError.h"
#include "WorkflowFactory.h"

using nlohmann::json;

namespace {

/** Owns every subsystem the facade needs. Member order matters: it is the construction (and reverse destruction) order. */
struct Engine {
    RouterRegistry routers;
    ProjectRegistry registry;
    WorkflowFactory factory;
    ReportService reports;
    DefinitionRepository definitions;
    ActionRegistry actions;
    WorkflowEngineFacade facade;

    Engine() : factory(&routers), facade(registry, factory, reports, definitions, actions, routers) {}
};

// ---------- printing helpers ----------

void banner(const std::string& title) {
    std::cout << "\n==============================================================\n"
              << "  " << title << "\n"
              << "==============================================================\n";
}

void step(const std::string& text) {
    std::cout << "\n--- " << text << "\n";
}

/** Runs fn and reports (instead of crashing on) the workflow errors we expect to hit on purpose. */
bool attempt(const std::string& what, const std::function<void()>& fn) {
    try {
        fn();
        return true;
    } catch (const std::exception& e) {
        std::cout << "  [rejected] " << what << "\n             " << e.what() << "\n";
        return false;
    }
}

const json* findItem(const json& project, const std::string& id) {
    const json& items = project["workItems"];
    for (std::size_t i = 0; i < items.size(); ++i) {
        if (items[i]["id"] == id) {
            return &items[i];
        }
    }
    return nullptr;
}

/** One line per task; '>' marks the tasks that can be worked on right now. */
void board(Engine& e, const std::string& pid) {
    json project = e.facade.exportProjectToJson(pid);
    std::vector<WorkItemView> available = e.facade.listAvailableWork(pid);

    std::cout << "  " << project["name"].get<std::string>() << "\n";
    const json& items = project["workItems"];
    if (items.empty()) {
        std::cout << "   (no tasks yet)\n";
    }
    for (std::size_t i = 0; i < items.size(); ++i) {
        const json& t = items[i];
        std::string id = t["id"].get<std::string>();
        bool ready = false;
        for (std::size_t k = 0; k < available.size(); ++k) {
            if (available[k].id == id) {
                ready = true;
            }
        }
        std::string who = t["executor"]["name"].get<std::string>();
        std::string due = t["deadline"].get<std::string>();
        std::cout << "   " << (ready ? ">" : " ") << " " << std::left << std::setw(12) << id
                  << std::setw(13) << t["status"].get<std::string>()
                  << std::setw(32) << t["name"].get<std::string>()
                  << std::setw(16) << (who.empty() ? "-" : who)
                  << (due.compare(0, 4, "0000") == 0 ? "" : due) << "\n";
    }
}

void history(Engine& e, const std::string& pid, std::size_t last) {
    std::vector<std::string> entries = e.facade.getHistory(pid);
    std::size_t shown = entries.size() < last ? entries.size() : last;
    std::cout << "  " << entries.size() << " audit entries recorded; the latest " << shown << ":\n";
    std::size_t from = entries.size() > last ? entries.size() - last : 0;
    for (std::size_t i = from; i < entries.size(); ++i) {
        std::string line = entries[i];
        for (std::size_t p = line.find('\n'); p != std::string::npos; p = line.find('\n', p + 1)) {
            line.replace(p, 1, " | ");
        }
        std::cout << "   * " << line << "\n";
    }
}

/** Runs a lifecycle action (start/complete/block/...) through the Command layer and prints the new state. */
void act(Engine& e, const std::string& pid, const std::string& tid, const std::string& action) {
    e.facade.performAction(pid, tid, action);
    std::cout << "  " << std::left << std::setw(9) << action << tid << "  ->  "
              << e.registry.findTask(pid, tid).getState()->toString() << "\n";
}

/** Decision tasks are not buildable from the template's branch data, so branch wiring is done here. */
void branch(Engine& e, const std::string& pid, const std::string& decisionId,
            const std::string& label, const std::string& targetId) {
    DecisionTask* d = dynamic_cast<DecisionTask*>(&e.registry.findComponent(pid, decisionId));
    d->addBranchTarget(label, &e.registry.findTask(pid, targetId));
}

json person(const std::string& id, const std::string& name, const std::string& role) {
    json p;
    p["id"] = id;
    p["name"] = name;
    p["role"] = role;
    return p;
}

json taskJson(const std::string& id, const std::string& name, const std::string& desc, const json& executor) {
    json t;
    t["type"] = "Task";
    t["id"] = id;
    t["name"] = name;
    t["description"] = desc;
    t["executor"] = executor;
    return t;
}

json decisionJson(const std::string& id, const std::string& name, const std::string& desc,
                  const json& executor, const std::string& router) {
    json t = taskJson(id, name, desc, executor);
    t["type"] = "Decision";
    t["router"] = router;
    return t;
}

json deadline(int y, int m, int d, int h, int min) {
    json j;
    j["year"] = y;
    j["month"] = m;
    j["day"] = d;
    j["hour"] = h;
    j["min"] = min;
    return j;
}

// ---------- scenario setup (shared by the scripted walkthroughs and the interactive menu) ----------

json adCampaignTemplate() {
    json lerato = person("AM-1", "Lerato Mokoena", "Account Manager");
    json sipho = person("CD-1", "Sipho Dlamini", "Creative Director");
    json alex = person("CW-1", "Alex van Wyk", "Copywriter");
    json maya = person("DS-1", "Maya Naidoo", "Designer");
    json producer = person("PR-1", "Jess Adams", "Producer");

    json tpl;
    tpl["type"] = "Project";
    tpl["name"] = "Ad Campaign";
    tpl["description"] = "Standard campaign from brief to launch";
    tpl["customFields"] = json::object();
    tpl["customFields"]["industry"] = "Advertising";
    tpl["workItems"] = json::array();
    tpl["workItems"].push_back(taskJson("A-BRIEF", "Client brief", "Capture goals, budget and audience", lerato));
    tpl["workItems"].push_back(taskJson("A-CONCEPT", "Creative concept", "Big idea and mood boards", sipho));
    tpl["workItems"].push_back(taskJson("A-COPY", "Write copy", "Headlines, body copy and taglines", alex));
    tpl["workItems"].push_back(taskJson("A-DESIGN", "Design visuals", "Key visuals and layouts", maya));
    tpl["workItems"].push_back(decisionJson("A-REVIEW", "Client review", "Client approves, asks for changes or rejects", lerato, "clientFeedback"));
    tpl["workItems"].push_back(taskJson("A-PRODUCE", "Produce final assets", "Print, video and digital cut-downs", producer));
    tpl["workItems"].push_back(taskJson("A-REVISE", "Revise creative", "Rework based on client notes", sipho));
    tpl["workItems"].push_back(taskJson("A-CLOSE", "Close out and archive", "Archive the rejected campaign", lerato));
    tpl["workItems"].push_back(taskJson("A-LAUNCH", "Launch campaign", "Go live on all channels", lerato));
    return tpl;
}

/**
 * Registers the routers and the ad campaign definition. Called once at startup: projects borrow their router,
 * so a router must never be replaced while a project that uses it is alive.
 */
void registerDefaults(Engine& e) {
    // Strategy: each router reads one field of the decision context to pick the branch.
    e.routers.add("severity", new FieldValueRoutingStrategy("severity"));
    e.routers.add("clientFeedback", new FieldValueRoutingStrategy("verdict"));

    e.definitions.add(WorkflowDefinition("ad-campaign", "Ad Campaign", "Advertising",
                                         "Brief, concept, parallel copy and design, client review, production, launch",
                                         adCampaignTemplate()));
}

/** Builds the ER admission workflow from a JSON template and wires its dependencies and branches. */
std::string setupHospital(Engine& e, const std::string& projectName) {
    WorkflowEngineFacade& f = e.facade;

    json nurse = person("N-01", "Thandi Nkosi", "Triage Nurse");
    json doctor = person("D-07", "Dr. Pillay", "ER Physician");
    json surgeon = person("D-12", "Dr. Mokoena", "Surgeon");
    json clerk = person("A-03", "Zanele Dube", "Admissions Clerk");

    json tpl;
    tpl["type"] = "Project";
    tpl["name"] = projectName;
    tpl["description"] = "Walk-in patient in the emergency room";
    tpl["customFields"] = json::object();
    tpl["customFields"]["department"] = "Emergency";
    tpl["customFields"]["priority"] = "high";
    tpl["workItems"] = json::array();
    tpl["workItems"].push_back(taskJson("H-TRIAGE", "Triage patient", "Record vitals and chief complaint", nurse));
    tpl["workItems"].push_back(taskJson("H-ASSESS", "Doctor assessment", "Examine patient and order tests", doctor));
    tpl["workItems"].push_back(decisionJson("H-DECIDE", "Severity routing", "Choose the care pathway", doctor, "severity"));
    tpl["workItems"].push_back(taskJson("H-SURGERY", "Emergency surgery", "Operate immediately", surgeon));
    tpl["workItems"].push_back(taskJson("H-WARD", "Admit to ward", "Observation overnight", nurse));
    tpl["workItems"].push_back(taskJson("H-DISCHARGE", "Discharge with prescription", "Send patient home", doctor));
    tpl["workItems"].push_back(taskJson("H-BILLING", "Billing and records", "Close the file", clerk));

    std::string pid = f.createFromTemplate(tpl);

    // Observer: assess after triage, decide after assess, billing after whichever care pathway is taken.
    f.addDependency(pid, "H-ASSESS", "H-TRIAGE");
    f.addDependency(pid, "H-DECIDE", "H-ASSESS");
    branch(e, pid, "H-DECIDE", "critical", "H-SURGERY");
    branch(e, pid, "H-DECIDE", "stable", "H-WARD");
    branch(e, pid, "H-DECIDE", "minor", "H-DISCHARGE");
    f.addDependency(pid, "H-BILLING", "H-SURGERY");
    f.addDependency(pid, "H-BILLING", "H-WARD");
    f.addDependency(pid, "H-BILLING", "H-DISCHARGE");
    return pid;
}

/** Starts an instance of the registered 'ad-campaign' definition and wires its dependencies and branches. */
std::string setupAgency(Engine& e, const std::string& instanceName) {
    WorkflowEngineFacade& f = e.facade;
    std::string pid = f.startInstance("ad-campaign", instanceName);

    // Copy and design run in parallel after the concept; the client review waits for both.
    f.addDependency(pid, "A-CONCEPT", "A-BRIEF");
    f.addDependency(pid, "A-COPY", "A-CONCEPT");
    f.addDependency(pid, "A-DESIGN", "A-CONCEPT");
    f.addDependency(pid, "A-REVIEW", "A-COPY");
    f.addDependency(pid, "A-REVIEW", "A-DESIGN");
    branch(e, pid, "A-REVIEW", "approved", "A-PRODUCE");
    branch(e, pid, "A-REVIEW", "changes", "A-REVISE");
    branch(e, pid, "A-REVIEW", "rejected", "A-CLOSE");
    f.addDependency(pid, "A-LAUNCH", "A-PRODUCE");
    return pid;
}

// ---------- scripted scenario 1: hospital ----------

void hospital(Engine& e) {
    banner("SCENARIO 1: HOSPITAL - Emergency room admission");
    WorkflowEngineFacade& f = e.facade;

    step("Build the ER admission workflow from a JSON template (Builder + Director) and wire its dependencies (Observer)");
    std::string pid = setupHospital(e, "ER Admission - Patient #4521");
    std::cout << "  created project " << pid << "\n";
    board(e, pid);
    std::cout << "  ('>' = ready to work on)\n";

    step("Try to jump ahead: the State pattern refuses to start a task whose prerequisites are not done");
    attempt("start H-ASSESS before triage", [&]() { f.performAction(pid, "H-ASSESS", "start"); });

    step("Edit a task (Memento) - set a deadline, then undo and redo it");
    json edit;
    edit["deadline"] = deadline(2026, 11, 2, 9, 30);
    edit["description"] = "Record vitals, allergies and chief complaint";
    f.editTask(pid, "H-TRIAGE", edit);
    std::cout << "  after edit : " << (*findItem(f.exportProjectToJson(pid), "H-TRIAGE"))["deadline"].get<std::string>() << "\n";
    f.undo(pid);
    std::cout << "  after undo : " << (*findItem(f.exportProjectToJson(pid), "H-TRIAGE"))["deadline"].get<std::string>() << "\n";
    f.redo(pid);
    std::cout << "  after redo : " << (*findItem(f.exportProjectToJson(pid), "H-TRIAGE"))["deadline"].get<std::string>() << "\n";

    step("Work the patient through triage and assessment");
    act(e, pid, "H-TRIAGE", "start");
    act(e, pid, "H-TRIAGE", "complete");
    act(e, pid, "H-ASSESS", "start");
    act(e, pid, "H-ASSESS", "complete");
    board(e, pid);

    step("Decision: the doctor routes the patient (Strategy). First with bad input, then properly");
    act(e, pid, "H-DECIDE", "start");
    attempt("decide without a severity", [&]() { f.decide(pid, "H-DECIDE", json::object()); });
    attempt("decide with an unknown pathway", [&]() {
        json ctx;
        ctx["severity"] = "unknown";
        f.decide(pid, "H-DECIDE", ctx);
    });
    json ctx;
    ctx["severity"] = "critical";
    std::cout << "  chosen branch: " << f.decide(pid, "H-DECIDE", ctx) << "\n";
    board(e, pid);
    std::cout << "  (the ward and discharge tasks were skipped automatically)\n";

    step("Surgery runs into a problem, gets blocked, resumes, and completes");
    act(e, pid, "H-SURGERY", "start");
    act(e, pid, "H-SURGERY", "block");
    act(e, pid, "H-SURGERY", "start");
    act(e, pid, "H-SURGERY", "complete");

    step("Oops - completion was recorded too early. Undo it, then redo it (Command)");
    std::cout << "  can undo: " << (f.canUndo(pid) ? "yes" : "no") << ", can redo: " << (f.canRedo(pid) ? "yes" : "no") << "\n";
    f.undo(pid);
    std::cout << "  undo   H-SURGERY  ->  " << e.registry.findTask(pid, "H-SURGERY").getState()->toString() << "\n";
    f.redo(pid);
    std::cout << "  redo   H-SURGERY  ->  " << e.registry.findTask(pid, "H-SURGERY").getState()->toString() << "\n";

    step("Billing is now unblocked");
    act(e, pid, "H-BILLING", "start");
    act(e, pid, "H-BILLING", "complete");

    step("Final status");
    std::cout << f.viewProjectStatus(pid) << "\n";
    step("Audit trail");
    history(e, pid, 5);

    step("Clone the workflow for the next patient (Prototype)");
    std::string next = f.cloneProject(pid, "ER Admission - Patient #4522");
    board(e, next);
}

// ---------- scripted scenario 2: ad agency ----------

void adAgency(Engine& e) {
    banner("SCENARIO 2: AD AGENCY - Campaign for a client");
    WorkflowEngineFacade& f = e.facade;

    step("Start an instance of the registered 'Ad Campaign' definition and wire its dependencies");
    std::string pid = setupAgency(e, "Zenith Sneakers - Spring Launch");
    std::cout << "  started instance " << pid << " from definition 'ad-campaign'\n";
    board(e, pid);

    step("Reassign the design work to a different designer (Memento edit through the facade)");
    f.assignExecutor(pid, "A-DESIGN", Participant("DS-2", "Kabelo Sithole", "Senior Designer"));
    std::cout << "  A-DESIGN is now assigned to "
              << (*findItem(f.exportProjectToJson(pid), "A-DESIGN"))["executor"]["name"].get<std::string>() << "\n";

    step("Brief and concept");
    act(e, pid, "A-BRIEF", "start");
    act(e, pid, "A-BRIEF", "complete");
    act(e, pid, "A-CONCEPT", "start");
    act(e, pid, "A-CONCEPT", "complete");
    board(e, pid);
    std::cout << "  (copy and design became ready at the same moment)\n";

    step("Copy and design in parallel; design is blocked waiting on client assets");
    act(e, pid, "A-COPY", "start");
    act(e, pid, "A-DESIGN", "start");
    act(e, pid, "A-DESIGN", "block");
    act(e, pid, "A-COPY", "complete");
    attempt("start the client review while design is unfinished", [&]() { f.performAction(pid, "A-REVIEW", "start"); });
    act(e, pid, "A-DESIGN", "start");
    act(e, pid, "A-DESIGN", "complete");

    step("Client review: the decision is routed by the client's verdict (Strategy)");
    act(e, pid, "A-REVIEW", "start");
    json verdict;
    verdict["verdict"] = "approved";
    std::cout << "  chosen branch: " << f.decide(pid, "A-REVIEW", verdict) << "\n";
    board(e, pid);

    step("Produce and launch");
    act(e, pid, "A-PRODUCE", "start");
    act(e, pid, "A-PRODUCE", "complete");
    act(e, pid, "A-LAUNCH", "start");
    act(e, pid, "A-LAUNCH", "complete");

    step("Final status");
    std::cout << f.viewProjectStatus(pid) << "\n";
    step("Audit trail");
    history(e, pid, 5);

    step("Reuse the finished campaign as a starting point for another client (Prototype)");
    std::string next = f.cloneProject(pid, "Brew & Co - Summer Promo");
    board(e, next);

    step("All projects in the engine");
    std::vector<std::string> ids = f.listProjects();
    for (std::size_t i = 0; i < ids.size(); ++i) {
        std::cout << "  " << ids[i] << ": " << e.registry.findProject(ids[i]).getName() << "\n";
    }
}

// ---------- interactive menu ----------

/** Thrown when stdin is closed (Ctrl+D or end of piped input) so the menu can exit cleanly. */
struct InputClosed {};

/** The engine plus the project the user is currently working on. */
struct Session {
    Engine& e;
    std::string pid;
    explicit Session(Engine& engine) : e(engine) {}
};

std::string trim(const std::string& s) {
    std::size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) {
        return "";
    }
    std::size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

/** Prompts and reads one line; an empty answer returns the fallback. */
std::string ask(const std::string& prompt, const std::string& fallback = "") {
    std::cout << prompt;
    if (!fallback.empty()) {
        std::cout << " [" << fallback << "]";
    }
    std::cout << ": " << std::flush;
    std::string line;
    if (!std::getline(std::cin, line)) {
        throw InputClosed();
    }
    line = trim(line);
    return line.empty() ? fallback : line;
}

/** Reads a whole number in [lo, hi]; returns -1 when the answer is blank or invalid. */
int askInt(const std::string& prompt, int lo, int hi) {
    std::string s = ask(prompt);
    if (s.empty()) {
        return -1;
    }
    char* end = nullptr;
    long v = std::strtol(s.c_str(), &end, 10);
    if (*end != '\0' || v < lo || v > hi) {
        std::cout << "  Please enter a number from " << lo << " to " << hi << ".\n";
        return -1;
    }
    return static_cast<int>(v);
}

/** Lists the tasks of the active project and lets the user pick one. Returns "" when cancelled. */
std::string pickTask(Session& s, const std::string& title, const std::string& onlyType = "") {
    json project = s.e.facade.exportProjectToJson(s.pid);
    const json& items = project["workItems"];
    std::vector<std::string> ids;

    std::cout << "\n" << title << "\n";
    for (std::size_t i = 0; i < items.size(); ++i) {
        if (!onlyType.empty() && items[i]["type"].get<std::string>() != onlyType) {
            continue;
        }
        ids.push_back(items[i]["id"].get<std::string>());
        std::cout << "  " << std::right << std::setw(2) << ids.size() << ") " << std::left << std::setw(12) << ids.back()
                  << std::setw(13) << items[i]["status"].get<std::string>()
                  << items[i]["name"].get<std::string>() << "\n";
    }
    if (ids.empty()) {
        std::cout << "  (no matching tasks)\n";
        return "";
    }
    int n = askInt("Choose a number (blank to cancel)", 1, static_cast<int>(ids.size()));
    return n < 0 ? "" : ids[static_cast<std::size_t>(n - 1)];
}

bool needProject(const Session& s) {
    if (s.pid.empty()) {
        std::cout << "  No active project yet. Load a scenario or create a project first (options 1-3).\n";
        return false;
    }
    return true;
}

/** Parses "YYYY-MM-DD" or "YYYY-MM-DD HH:MM" into the deadline payload expected by editTask. */
bool parseDeadline(const std::string& text, json& out) {
    int y = 0, m = 0, d = 0, h = 0, mi = 0;
    int got = std::sscanf(text.c_str(), "%d-%d-%d %d:%d", &y, &m, &d, &h, &mi);
    if (got != 3 && got != 5) {
        return false;
    }
    if (y < 1900 || m < 1 || m > 12 || d < 1 || d > 31 || h < 0 || h > 23 || mi < 0 || mi > 59) {
        return false;
    }
    out = deadline(y, m, d, h, mi);
    return true;
}

void showMenu(Session& s) {
    std::cout << "\n------------------------------------------------------------\n";
    if (s.pid.empty()) {
        std::cout << " Active project: (none)\n";
    } else {
        std::cout << " Active project: " << s.pid << "  " << s.e.registry.findProject(s.pid).getName()
                  << "   [undo: " << (s.e.facade.canUndo(s.pid) ? "yes" : "no")
                  << ", redo: " << (s.e.facade.canRedo(s.pid) ? "yes" : "no") << "]\n";
    }
    std::cout << "------------------------------------------------------------\n"
              << " PROJECTS                     VIEW\n"
              << "  1) Load hospital scenario     6) Task board\n"
              << "  2) Load ad agency scenario    7) Status report\n"
              << "  3) New blank project          8) Audit history\n"
              << "  4) Switch project             9) Export as JSON\n"
              << "  5) Clone active project\n"
              << " WORK                         EDIT\n"
              << " 10) Run an action on a task   14) Add a task\n"
              << " 11) Make a decision           15) Add a dependency\n"
              << " 12) Undo                      16) Assign an executor\n"
              << " 13) Redo                      17) Edit a task\n"
              << "  0) Quit\n";
}

void menuLoadHospital(Session& s) {
    std::string who = ask("Patient name or case number", "#4521");
    s.pid = setupHospital(s.e, "ER Admission - " + who);
    std::cout << "  Created " << s.pid << ". Start with triage, then follow the '>' markers.\n";
    board(s.e, s.pid);
}

void menuLoadAgency(Session& s) {
    std::string client = ask("Client and campaign name", "Zenith Sneakers - Spring Launch");
    s.pid = setupAgency(s.e, client);
    std::cout << "  Created " << s.pid << " from the 'ad-campaign' definition.\n";
    board(s.e, s.pid);
}

void menuNewProject(Session& s) {
    std::string name = ask("Project name");
    if (name.empty()) {
        std::cout << "  Cancelled.\n";
        return;
    }
    std::string desc = ask("Description (optional)");
    s.pid = s.e.facade.createProject(name, desc);
    std::cout << "  Created " << s.pid << ". Use 'Add a task' (14) to fill it.\n";
}

void menuSwitch(Session& s) {
    std::vector<std::string> ids = s.e.facade.listProjects();
    if (ids.empty()) {
        std::cout << "  No projects yet.\n";
        return;
    }
    std::cout << "\nProjects:\n";
    for (std::size_t i = 0; i < ids.size(); ++i) {
        std::cout << "  " << std::right << std::setw(2) << (i + 1) << ") " << std::left << std::setw(8) << ids[i]
                  << s.e.registry.findProject(ids[i]).getName() << (ids[i] == s.pid ? "   <- active" : "") << "\n";
    }
    int n = askInt("Choose a number (blank to cancel)", 1, static_cast<int>(ids.size()));
    if (n > 0) {
        s.pid = ids[static_cast<std::size_t>(n - 1)];
        board(s.e, s.pid);
    }
}

void menuClone(Session& s) {
    if (!needProject(s)) {
        return;
    }
    std::string name = ask("Name for the copy", s.e.registry.findProject(s.pid).getName() + " (copy)");
    s.pid = s.e.facade.cloneProject(s.pid, name);
    std::cout << "  Cloned. The copy " << s.pid << " is now active (every task starts as Pending).\n";
    board(s.e, s.pid);
}

void menuAction(Session& s) {
    if (!needProject(s)) {
        return;
    }
    std::string tid = pickTask(s, "Which task?");
    if (tid.empty()) {
        return;
    }
    std::vector<std::string> names = s.e.actions.names();
    std::cout << "\nActions:\n";
    for (std::size_t i = 0; i < names.size(); ++i) {
        std::cout << "  " << (i + 1) << ") " << names[i] << "\n";
    }
    int n = askInt("Choose an action (blank to cancel)", 1, static_cast<int>(names.size()));
    if (n < 0) {
        return;
    }
    act(s.e, s.pid, tid, names[static_cast<std::size_t>(n - 1)]);
    board(s.e, s.pid);
}

void menuDecide(Session& s) {
    if (!needProject(s)) {
        return;
    }
    std::string tid = pickTask(s, "Which decision task? (it must be started first)", "Decision");
    if (tid.empty()) {
        return;
    }
    // a decision can only be made once the task is In Progress; say so up front instead of failing after the prompts
    std::string state = s.e.registry.findTask(s.pid, tid).getState()->toString();
    if (state != "In Progress") {
        std::cout << "  " << tid << " is " << state << ". Start it first (option 10, action 'start'), then decide.\n";
        return;
    }
    DecisionTask* d = dynamic_cast<DecisionTask*>(&s.e.registry.findComponent(s.pid, tid));
    std::vector<std::string> labels;
    const std::map<std::string, std::vector<Task*> >& branches = d->getBranches();
    for (std::map<std::string, std::vector<Task*> >::const_iterator it = branches.begin(); it != branches.end(); ++it) {
        labels.push_back(it->first);
    }

    std::cout << "\nBranches:\n";
    for (std::size_t i = 0; i < labels.size(); ++i) {
        std::cout << "  " << (i + 1) << ") " << labels[i] << "\n";
    }
    std::string answer = ask("Choose a number, or type a branch name (blank to cancel)");
    if (answer.empty()) {
        return;
    }
    char* end = nullptr;
    long idx = std::strtol(answer.c_str(), &end, 10);
    if (*end == '\0' && idx >= 1 && idx <= static_cast<long>(labels.size())) {
        answer = labels[static_cast<std::size_t>(idx - 1)];
    }

    // Each scenario's router reads a different context field ("severity" or "verdict"); supply both.
    json context;
    context["severity"] = answer;
    context["verdict"] = answer;
    // decide first, print after - printing "Chosen branch:" in the same << chain showed it even when decide() threw
    std::string chosen = s.e.facade.decide(s.pid, tid, context);
    std::cout << "  Chosen branch: " << chosen << "\n";
    board(s.e, s.pid);
}

void menuUndoRedo(Session& s, bool undo) {
    if (!needProject(s)) {
        return;
    }
    if (undo ? !s.e.facade.canUndo(s.pid) : !s.e.facade.canRedo(s.pid)) {
        std::cout << "  Nothing to " << (undo ? "undo" : "redo") << ".\n";
        return;
    }
    if (undo) {
        s.e.facade.undo(s.pid);
    } else {
        s.e.facade.redo(s.pid);
    }
    std::cout << "  Done.\n";
    board(s.e, s.pid);
}

void menuAddTask(Session& s) {
    if (!needProject(s)) {
        return;
    }
    std::string name = ask("Task name (blank to cancel)");
    if (name.empty()) {
        return;
    }
    std::string desc = ask("Description (optional)");
    std::string id = s.e.facade.addTaskToProject(s.pid, name, desc);
    std::cout << "  Added task " << id << ".\n";
    board(s.e, s.pid);
}

void menuAddDependency(Session& s) {
    if (!needProject(s)) {
        return;
    }
    std::string dependent = pickTask(s, "Which task must WAIT?");
    if (dependent.empty()) {
        return;
    }
    std::string prerequisite = pickTask(s, "...until which task is done?");
    if (prerequisite.empty()) {
        return;
    }
    s.e.facade.addDependency(s.pid, dependent, prerequisite);
    std::cout << "  " << dependent << " now waits for " << prerequisite << ".\n";
    board(s.e, s.pid);
}

void menuAssign(Session& s) {
    if (!needProject(s)) {
        return;
    }
    std::string tid = pickTask(s, "Assign an executor to which task?");
    if (tid.empty()) {
        return;
    }
    std::string name = ask("Executor name (blank to cancel)");
    if (name.empty()) {
        return;
    }
    std::string id = ask("Executor id", "U-" + name);
    std::string role = ask("Role (optional)");
    s.e.facade.assignExecutor(s.pid, tid, Participant(id, name, role));
    board(s.e, s.pid);
}

void menuEdit(Session& s) {
    if (!needProject(s)) {
        return;
    }
    std::string tid = pickTask(s, "Edit which task?");
    if (tid.empty()) {
        return;
    }
    std::cout << "\nWhat to change?\n  1) Name\n  2) Description\n  3) Deadline\n  4) Custom field\n";
    int n = askInt("Choose (blank to cancel)", 1, 4);
    if (n < 0) {
        return;
    }

    json payload;
    if (n == 1 || n == 2) {
        std::string value = ask(n == 1 ? "New name" : "New description");
        if (value.empty()) {
            std::cout << "  Cancelled.\n";
            return;
        }
        payload[n == 1 ? "name" : "description"] = value;
    } else if (n == 3) {
        json when;
        if (!parseDeadline(ask("Deadline as YYYY-MM-DD or YYYY-MM-DD HH:MM"), when)) {
            std::cout << "  That is not a valid date. Nothing changed.\n";
            return;
        }
        payload["deadline"] = when;
    } else {
        std::string key = ask("Field name");
        std::string value = ask("Value");
        if (key.empty()) {
            std::cout << "  Cancelled.\n";
            return;
        }
        payload["customFields"] = json::object();
        payload["customFields"][key] = value;
    }
    s.e.facade.editTask(s.pid, tid, payload);
    std::cout << "  Updated. (Undo with option 12.)\n";
    board(s.e, s.pid);
}

/** Runs the menu until the user quits or input ends. Workflow errors are shown and the menu carries on. */
void interactive(Engine& e) {
    banner("WORKFLOW ENGINE - interactive mode");
    std::cout << "  Load a scenario (1 or 2), then use the menu to run the workflow yourself.\n"
              << "  Tip: on every list, a blank answer cancels.\n";

    Session s(e);
    try {
        for (;;) {
            showMenu(s);
            std::string choice = ask("Select an option");
            if (choice == "0" || choice == "q" || choice == "Q") {
                std::cout << "  Goodbye.\n";
                return;
            }
            try {
                if (choice == "1") menuLoadHospital(s);
                else if (choice == "2") menuLoadAgency(s);
                else if (choice == "3") menuNewProject(s);
                else if (choice == "4") menuSwitch(s);
                else if (choice == "5") menuClone(s);
                else if (choice == "6") { if (needProject(s)) board(e, s.pid); }
                else if (choice == "7") { if (needProject(s)) std::cout << e.facade.viewProjectStatus(s.pid) << "\n"; }
                else if (choice == "8") { if (needProject(s)) history(e, s.pid, 1000); }
                else if (choice == "9") { if (needProject(s)) std::cout << e.facade.exportProjectToJson(s.pid).dump(2) << "\n"; }
                else if (choice == "10") menuAction(s);
                else if (choice == "11") menuDecide(s);
                else if (choice == "12") menuUndoRedo(s, true);
                else if (choice == "13") menuUndoRedo(s, false);
                else if (choice == "14") menuAddTask(s);
                else if (choice == "15") menuAddDependency(s);
                else if (choice == "16") menuAssign(s);
                else if (choice == "17") menuEdit(s);
                else std::cout << "  Unknown option '" << choice << "'. Enter a number from the menu.\n";
            } catch (const InputClosed&) {
                throw;
            } catch (const std::exception& ex) {
                std::cout << "  [rejected] " << ex.what() << "\n";
            }
        }
    } catch (const InputClosed&) {
        std::cout << "\n  Input closed. Goodbye.\n";
    }
}

} // namespace

int main(int argc, char** argv) {
    std::string mode = argc > 1 ? argv[1] : "menu";
    if (mode != "menu" && mode != "interactive" && mode != "hospital" && mode != "agency" && mode != "all") {
        std::cerr << "Usage: " << argv[0] << " [menu|hospital|agency|all]\n"
                  << "  (no argument starts the interactive menu)\n";
        return 2;
    }

    try {
        Engine engine;
        registerDefaults(engine);
        if (mode == "menu" || mode == "interactive") {
            interactive(engine);
        } else {
            if (mode == "hospital" || mode == "all") {
                hospital(engine);
            }
            if (mode == "agency" || mode == "all") {
                adAgency(engine);
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "\nUnexpected error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
