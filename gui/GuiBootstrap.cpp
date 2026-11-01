#include "gui/GuiBootstrap.h"

#include <nlohmann/json.hpp>

#include "RoutingStrategy.h"
#include "Support.h"

namespace {

using nlohmann::json;

json person(const std::string& id, const std::string& name, const std::string& role) {
    json p;
    p["id"] = id;
    p["name"] = name;
    p["role"] = role;
    return p;
}

json task(const std::string& id, const std::string& name, const std::string& desc, const json& executor,
          const json& dependsOn = json::array()) {
    json t;
    t["type"] = "Task";
    t["id"] = id;
    t["name"] = name;
    t["description"] = desc;
    t["executor"] = executor;
    t["dependsOn"] = dependsOn;
    return t;
}

// "inputs" is just a hint for the board about which field the router reads
json decision(const std::string& id, const std::string& name, const std::string& desc, const json& executor,
              const json& dependsOn, const std::string& router, const std::string& inputField, const json& branches) {
    json t = task(id, name, desc, executor, dependsOn);
    t["type"] = "Decision";
    t["router"] = router;
    t["customFields"] = json::object();
    t["customFields"]["inputs"] = inputField;
    t["branches"] = branches;
    return t;
}

json ids(const std::string& a) {
    json j = json::array();
    j.push_back(a);
    return j;
}

json ids(const std::string& a, const std::string& b) {
    json j = ids(a);
    j.push_back(b);
    return j;
}

json ids(const std::string& a, const std::string& b, const std::string& c) {
    json j = ids(a, b);
    j.push_back(c);
    return j;
}

json hospitalTemplate() {
    json nurse = person("N-01", "Thandi Nkosi", "Triage Nurse");
    json doctor = person("D-07", "Dr. Pillay", "ER Physician");
    json surgeon = person("D-12", "Dr. Mokoena", "Surgeon");
    json clerk = person("A-03", "Zanele Dube", "Admissions Clerk");

    json branches;
    branches["critical"] = ids("H-SURGERY");
    branches["stable"] = ids("H-WARD");
    branches["minor"] = ids("H-DISCHARGE");

    json tpl;
    tpl["type"] = "Project";
    tpl["name"] = "ER Admission";
    tpl["description"] = "Walk-in patient in the emergency room";
    tpl["customFields"] = json::object();
    tpl["customFields"]["industry"] = "Healthcare";
    tpl["workItems"] = json::array();
    tpl["workItems"].push_back(task("H-TRIAGE", "Triage patient", "Record vitals and chief complaint", nurse));
    tpl["workItems"].push_back(task("H-ASSESS", "Doctor assessment", "Examine patient and order tests", doctor, ids("H-TRIAGE")));
    tpl["workItems"].push_back(decision("H-DECIDE", "Severity routing", "Choose the care pathway", doctor, ids("H-ASSESS"),
                                        "severity", "severity", branches));
    tpl["workItems"].push_back(task("H-SURGERY", "Emergency surgery", "Operate immediately", surgeon, ids("H-DECIDE")));
    tpl["workItems"].push_back(task("H-WARD", "Admit to ward", "Observation overnight", nurse, ids("H-DECIDE")));
    tpl["workItems"].push_back(task("H-DISCHARGE", "Discharge with prescription", "Send patient home", doctor, ids("H-DECIDE")));
    tpl["workItems"].push_back(task("H-BILLING", "Billing and records", "Close the file", clerk,
                                    ids("H-SURGERY", "H-WARD", "H-DISCHARGE")));
    return tpl;
}

json adCampaignTemplate() {
    json lerato = person("AM-1", "Lerato Mokoena", "Account Manager");
    json sipho = person("CD-1", "Sipho Dlamini", "Creative Director");
    json alex = person("CW-1", "Alex van Wyk", "Copywriter");
    json maya = person("DS-1", "Maya Naidoo", "Designer");
    json producer = person("PR-1", "Jess Adams", "Producer");

    json branches;
    branches["approved"] = ids("A-PRODUCE");
    branches["changes"] = ids("A-REVISE");
    branches["rejected"] = ids("A-CLOSE");

    json tpl;
    tpl["type"] = "Project";
    tpl["name"] = "Ad Campaign";
    tpl["description"] = "Standard campaign from brief to launch";
    tpl["customFields"] = json::object();
    tpl["customFields"]["industry"] = "Advertising";
    tpl["workItems"] = json::array();
    tpl["workItems"].push_back(task("A-BRIEF", "Client brief", "Capture goals, budget and audience", lerato));
    tpl["workItems"].push_back(task("A-CONCEPT", "Creative concept", "Big idea and mood boards", sipho, ids("A-BRIEF")));
    tpl["workItems"].push_back(task("A-COPY", "Write copy", "Headlines, body copy and taglines", alex, ids("A-CONCEPT")));
    tpl["workItems"].push_back(task("A-DESIGN", "Design visuals", "Key visuals and layouts", maya, ids("A-CONCEPT")));
    tpl["workItems"].push_back(decision("A-REVIEW", "Client review", "Client approves, asks for changes or rejects", lerato,
                                        ids("A-COPY", "A-DESIGN"), "clientFeedback", "verdict", branches));
    tpl["workItems"].push_back(task("A-PRODUCE", "Produce final assets", "Print, video and digital cut-downs", producer, ids("A-REVIEW")));
    tpl["workItems"].push_back(task("A-REVISE", "Revise creative", "Rework based on client notes", sipho, ids("A-REVIEW")));
    tpl["workItems"].push_back(task("A-CLOSE", "Close out and archive", "Archive the rejected campaign", lerato, ids("A-REVIEW")));
    tpl["workItems"].push_back(task("A-LAUNCH", "Launch campaign", "Go live on all channels", lerato, ids("A-PRODUCE")));
    return tpl;
}

}  // namespace

GuiEngine::GuiEngine()
    : factory(&routers), facade(registry, factory, reports, definitions, actions, routers) {
    routers.add("severity", new FieldValueRoutingStrategy("severity"));
    routers.add("clientFeedback", new FieldValueRoutingStrategy("verdict"));

    definitions.add(WorkflowDefinition("er-admission", "ER Admission", "Healthcare",
                                       "Emergency room admission: triage, assessment, severity routing, billing",
                                       hospitalTemplate()));
    definitions.add(WorkflowDefinition("ad-campaign", "Ad Campaign", "Advertising",
                                       "Brief, concept, parallel copy and design, client review, production, launch",
                                       adCampaignTemplate()));
}
