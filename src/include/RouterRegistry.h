/**
 * @file RouterRegistry.h
 * @brief Owner and lookup table for routing strategies.
 */
#pragma once

#include <map>
#include <string>
#include <vector>

class RoutingStrategy;

/**
 * @brief Registry mapping router keys to routing strategies.
 *
 * Owns every registered strategy and deletes them on destruction. Templates refer to a
 * router by key (e.g. "router": "claimOutcome"); the Director resolves the key here and
 * gives the `DecisionTask` a borrowed pointer.
 */
class RouterRegistry {
private:
    std::map<std::string, RoutingStrategy*> routers;  ///< Strategies by key. OWNS every value.

    RouterRegistry(const RouterRegistry&) = delete;
    RouterRegistry& operator=(const RouterRegistry&) = delete;

public:
    /**
     * @brief Constructs an empty registry.
     */
    RouterRegistry();

    /**
     * @brief Deletes every registered strategy.
     */
    ~RouterRegistry();

    /**
     * @brief Registers a strategy under a key. TAKES ownership of r.
     *
     * An existing strategy under the same key is deleted and replaced.
     *
     * @param key The string identifier templates use to refer to the router.
     * @param r Raw pointer to a newly created strategy.
     * @throws `std::logic_error` If r is null or is already registered under another key.
     */
    void add(const std::string& key, RoutingStrategy* r);

    /**
     * @brief Looks up a strategy by key.
     *
     * @param key The string identifier of the router.
     * @return `RoutingStrategy*` Borrowed pointer; the registry keeps ownership.
     * @throws `NotFoundError` If no strategy is registered under key.
     */
    RoutingStrategy* get(const std::string& key) const;

    /**
     * @brief Checks if a key is registered.
     *
     * @param key The string identifier of the router.
     * @return `bool` `true` If a strategy exists under key, `false` otherwise.
     */
    bool has(const std::string& key) const;

    /**
     * @brief Retrieves all registered keys.
     *
     * @return `std::vector<std::string>` Registered router keys in alphabetical order.
     */
    std::vector<std::string> keys() const;
};
