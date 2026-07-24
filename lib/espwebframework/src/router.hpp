/**
 * @file router.hpp
 * @brief ESP32 Web Framework - URL Router
 * 
 * URL dispatcher with support for:
 * - Dynamic path parameters
 * - Multiple HTTP methods
 * - Static file directories
 * - Trie-based routing for fast lookup
 * 
 * @author ESP32 Web Framework
 * @version 1.0.0
 */

#ifndef ESP_WEB_FRAMEWORK_ROUTER_HPP
#define ESP_WEB_FRAMEWORK_ROUTER_HPP

#include <Arduino.h>
#include <map>
#include <vector>
#include <memory>
#include <functional>

#include "http.hpp"

namespace espweb {

/**
 * @brief Route parameter type for validation
 */
enum class ParamType {
    STRING,     ///< Any string
    INT,        ///< Integer value
    FLOAT,      ///< Float value
    UUID        ///< UUID format
};

/**
 * @brief Route parameter definition
 */
struct RouteParam {
    String name;
    ParamType type = ParamType::STRING;
};

/**
 * @brief Route definition
 */
struct Route {
    String pattern;                          ///< URL pattern (e.g., /user/<id:int>)
    HttpMethod method;                       ///< HTTP method
    ViewHandler handler;                     ///< View handler function
    std::vector<RouteParam> params;          ///< Extracted parameters
    String name;                             ///< Route name for reverse lookup
};

/**
 * @brief Static directory mapping
 */
struct StaticDir {
    String urlPath;                          ///< URL prefix (e.g., /static)
    String fsPath;                           ///< Filesystem path (e.g., /sd/static)
    bool enableCache = true;                 ///< Enable caching
};

/**
 * @brief Trie node for fast route matching
 */
struct TrieNode {
    std::map<String, std::unique_ptr<TrieNode>> children;
    std::map<HttpMethod, Route> routes;
    bool isParam = false;
    RouteParam paramInfo;
};

/**
 * @brief URL Router class
 * 
 * Handles URL routing and dispatching to view handlers.
 * Uses trie-based routing for O(n) lookup where n is path depth.
 */
class Router {
public:
    Router();
    ~Router() = default;
    
    /**
     * @brief Add GET route
     * @param pattern URL pattern
     * @param handler View handler
     * @param name Optional route name
     * @return Reference to this for chaining
     */
    Router& get(const String& pattern, ViewHandler handler, const String& name = "");
    
    /**
     * @brief Add POST route
     * @param pattern URL pattern
     * @param handler View handler
     * @param name Optional route name
     * @return Reference to this for chaining
     */
    Router& post(const String& pattern, ViewHandler handler, const String& name = "");
    
    /**
     * @brief Add PUT route
     * @param pattern URL pattern
     * @param handler View handler
     * @param name Optional route name
     * @return Reference to this for chaining
     */
    Router& put(const String& pattern, ViewHandler handler, const String& name = "");
    
    /**
     * @brief Add DELETE route
     * @param pattern URL pattern
     * @param handler View handler
     * @param name Optional route name
     * @return Reference to this for chaining
     */
    Router& del(const String& pattern, ViewHandler handler, const String& name = "");
    
    /**
     * @brief Add PATCH route
     * @param pattern URL pattern
     * @param handler View handler
     * @param name Optional route name
     * @return Reference to this for chaining
     */
    Router& patch(const String& pattern, ViewHandler handler, const String& name = "");
    
    /**
     * @brief Add route with any method
     * @param pattern URL pattern
     * @param method HTTP method
     * @param handler View handler
     * @param name Optional route name
     * @return Reference to this for chaining
     */
    Router& add(const String& pattern, HttpMethod method, ViewHandler handler, const String& name = "");
    
    /**
     * @brief Add static directory mapping
     * @param urlPath URL prefix
     * @param fsPath Filesystem path
     * @param enableCache Enable caching
     * @return Reference to this for chaining
     */
    Router& staticDir(const String& urlPath, const String& fsPath, bool enableCache = true);
    
    /**
     * @brief Match request to route
     * @param method HTTP method
     * @param path URL path
     * @param params Output parameter map
     * @return Pointer to matched Route or nullptr
     */
    const Route* match(HttpMethod method, const String& path, std::map<String, String>& params);
    
    /**
     * @brief Check if path matches static directory
     * @param path URL path
     * @param fsPath Output filesystem path
     * @return true if static file
     */
    bool matchStatic(const String& path, String& fsPath);
    
    /**
     * @brief Get URL for named route
     * @param name Route name
     * @param params URL parameters
     * @return Generated URL or empty string
     */
    String reverse(const String& name, const std::map<String, String>& params = {});
    
    /**
     * @brief Get all registered routes
     * @return Vector of routes
     */
    const std::vector<Route>& getRoutes() const { return routes_; }
    
    /**
     * @brief Get all static directories
     * @return Vector of static directories
     */
    const std::vector<StaticDir>& getStaticDirs() const { return staticDirs_; }
    
    /**
     * @brief Clear all routes
     */
    void clear();
    
private:
    /**
     * @brief Parse URL pattern and extract parameters
     * @param pattern URL pattern
     * @param params Output parameter vector
     * @return Parsed pattern segments
     */
    std::vector<String> parsePattern(const String& pattern, std::vector<RouteParam>& params);
    
    /**
     * @brief Add route to trie
     * @param route Route to add
     */
    void addToTrie(const Route& route);
    
    /**
     * @brief Validate parameter value
     * @param value Parameter value
     * @param type Expected type
     * @return true if valid
     */
    bool validateParam(const String& value, ParamType type);
    
    std::unique_ptr<TrieNode> root_;
    std::vector<Route> routes_;
    std::vector<StaticDir> staticDirs_;
    std::map<String, size_t> namedRoutes_;
};

/**
 * @brief Helper function to get global Router instance
 */
inline Router& Routes() {
    static Router instance;
    return instance;
}

} // namespace espweb

#endif // ESP_WEB_FRAMEWORK_ROUTER_HPP
