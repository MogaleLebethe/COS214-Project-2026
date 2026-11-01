/**
 * @file RouterRegistry.cpp
 * @brief Implementation of RouterRegistry.
 */
#include "RouterRegistry.h"
#include "RoutingStrategy.h"
#include "WorkflowError.h"

#include <stdexcept>

RouterRegistry::RouterRegistry() {}

RouterRegistry::~RouterRegistry() {
    for (std::map<std::string, RoutingStrategy*>::iterator it = routers.begin(); it != routers.end(); ++it)
        delete it->second;
}

void RouterRegistry::add(const std::string& key, RoutingStrategy* r) {
    if (!r) throw std::logic_error("RouterRegistry::add: null strategy for " + key);

    for (std::map<std::string, RoutingStrategy*>::iterator it = routers.begin(); it != routers.end(); ++it)
        if (it->second == r && it->first != key)
            throw std::logic_error("RouterRegistry::add: strategy already registered as " + it->first);

    std::map<std::string, RoutingStrategy*>::iterator existing = routers.find(key);
    if (existing == routers.end()) {
        routers[key] = r;
    } else if (existing->second != r) {
        delete existing->second;
        existing->second = r;
    }
}

RoutingStrategy* RouterRegistry::get(const std::string& key) const {
    std::map<std::string, RoutingStrategy*>::const_iterator it = routers.find(key);
    if (it == routers.end()) throw NotFoundError("Unknown router: " + key);
    return it->second;
}

bool RouterRegistry::has(const std::string& key) const {
    return routers.find(key) != routers.end();
}

std::vector<std::string> RouterRegistry::keys() const {
    std::vector<std::string> out;
    for (std::map<std::string, RoutingStrategy*>::const_iterator it = routers.begin(); it != routers.end(); ++it)
        out.push_back(it->first);
    return out;
}
