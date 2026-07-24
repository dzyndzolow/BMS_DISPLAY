/**
 * @file router.cpp
 * @brief ESP32 Web Framework - Router Implementation
 * 
 * @author ESP32 Web Framework
 * @version 1.0.0
 */

#include "router.hpp"
#include "logger.hpp"

namespace espweb {

Router::Router() {
    root_ = std::make_unique<TrieNode>();
}

Router& Router::get(const String& pattern, ViewHandler handler, const String& name) {
    return add(pattern, HttpMethod::GET, handler, name);
}

Router& Router::post(const String& pattern, ViewHandler handler, const String& name) {
    return add(pattern, HttpMethod::POST, handler, name);
}

Router& Router::put(const String& pattern, ViewHandler handler, const String& name) {
    return add(pattern, HttpMethod::PUT, handler, name);
}

Router& Router::del(const String& pattern, ViewHandler handler, const String& name) {
    return add(pattern, HttpMethod::DELETE, handler, name);
}

Router& Router::patch(const String& pattern, ViewHandler handler, const String& name) {
    return add(pattern, HttpMethod::PATCH, handler, name);
}

Router& Router::add(const String& pattern, HttpMethod method, ViewHandler handler, const String& name) {
    Route route;
    route.pattern = pattern;
    route.method = method;
    route.handler = handler;
    route.name = name;
    
    // Parse pattern for parameters
    parsePattern(pattern, route.params);
    
    // Add to routes list
    routes_.push_back(route);
    
    // Add to named routes if name provided
    if (!name.isEmpty()) {
        namedRoutes_[name] = routes_.size() - 1;
    }
    
    // Add to trie
    addToTrie(route);
    
    LOG_DEBUG("Router", "Added route: " + String(methodToString(method)) + " " + pattern);
    
    return *this;
}

Router& Router::staticDir(const String& urlPath, const String& fsPath, bool enableCache) {
    StaticDir dir;
    dir.urlPath = urlPath;
    dir.fsPath = fsPath;
    dir.enableCache = enableCache;
    
    staticDirs_.push_back(dir);
    
    LOG_DEBUG("Router", "Added static dir: " + urlPath + " -> " + fsPath);
    
    return *this;
}

std::vector<String> Router::parsePattern(const String& pattern, std::vector<RouteParam>& params) {
    std::vector<String> segments;
    String current;
    
    for (size_t i = 0; i < pattern.length(); i++) {
        char c = pattern.charAt(i);
        
        if (c == '/') {
            if (!current.isEmpty()) {
                segments.push_back(current);
                current = "";
            }
        } else if (c == '<') {
            // Start of parameter
            i++;
            String paramStr;
            while (i < pattern.length() && pattern.charAt(i) != '>') {
                paramStr += pattern.charAt(i);
                i++;
            }
            
            RouteParam param;
            int colonIdx = paramStr.indexOf(':');
            if (colonIdx > 0) {
                param.name = paramStr.substring(0, colonIdx);
                String typeStr = paramStr.substring(colonIdx + 1);
                if (typeStr == "int") param.type = ParamType::INT;
                else if (typeStr == "float") param.type = ParamType::FLOAT;
                else if (typeStr == "uuid") param.type = ParamType::UUID;
                else param.type = ParamType::STRING;
            } else {
                param.name = paramStr;
                param.type = ParamType::STRING;
            }
            
            params.push_back(param);
            segments.push_back("<" + param.name + ">");
        } else {
            current += c;
        }
    }
    
    if (!current.isEmpty()) {
        segments.push_back(current);
    }
    
    return segments;
}

void Router::addToTrie(const Route& route) {
    std::vector<RouteParam> params;
    std::vector<String> segments = parsePattern(route.pattern, params);
    
    TrieNode* node = root_.get();
    size_t paramIdx = 0;
    
    for (const String& segment : segments) {
        if (segment.startsWith("<")) {
            // Parameter segment
            String paramKey = "<param>";
            
            if (node->children.find(paramKey) == node->children.end()) {
                node->children[paramKey] = std::make_unique<TrieNode>();
                node->children[paramKey]->isParam = true;
                if (paramIdx < params.size()) {
                    node->children[paramKey]->paramInfo = params[paramIdx];
                }
            }
            node = node->children[paramKey].get();
            paramIdx++;
        } else {
            // Static segment
            if (node->children.find(segment) == node->children.end()) {
                node->children[segment] = std::make_unique<TrieNode>();
            }
            node = node->children[segment].get();
        }
    }
    
    // Store route at leaf node
    node->routes[route.method] = route;
}

const Route* Router::match(HttpMethod method, const String& path, 
                          std::map<String, String>& params) {
    // Split path into segments
    std::vector<String> segments;
    String current;
    
    for (size_t i = 0; i < path.length(); i++) {
        char c = path.charAt(i);
        if (c == '/' || c == '?') {
            if (!current.isEmpty()) {
                segments.push_back(current);
                current = "";
            }
            if (c == '?') break; // Stop at query string
        } else {
            current += c;
        }
    }
    if (!current.isEmpty()) {
        segments.push_back(current);
    }
    
    // Traverse trie
    TrieNode* node = root_.get();
    std::vector<std::pair<String, String>> extractedParams;
    
    for (const String& segment : segments) {
        // Try exact match first
        if (node->children.find(segment) != node->children.end()) {
            node = node->children[segment].get();
        }
        // Try parameter match
        else if (node->children.find("<param>") != node->children.end()) {
            TrieNode* paramNode = node->children["<param>"].get();
            
            // Validate parameter type
            if (validateParam(segment, paramNode->paramInfo.type)) {
                extractedParams.push_back({paramNode->paramInfo.name, segment});
                node = paramNode;
            } else {
                return nullptr; // Type validation failed
            }
        }
        else {
            return nullptr; // No match
        }
    }
    
    // Check if route exists for method
    if (node->routes.find(method) != node->routes.end()) {
        // Copy extracted params
        for (const auto& p : extractedParams) {
            params[p.first] = p.second;
        }
        return &node->routes[method];
    }
    
    return nullptr;
}

bool Router::matchStatic(const String& path, String& fsPath) {
    for (const auto& dir : staticDirs_) {
        if (path.startsWith(dir.urlPath)) {
            fsPath = dir.fsPath + path.substring(dir.urlPath.length());
            return true;
        }
    }
    return false;
}

String Router::reverse(const String& name, const std::map<String, String>& params) {
    auto it = namedRoutes_.find(name);
    if (it == namedRoutes_.end()) {
        return "";
    }
    
    const Route& route = routes_[it->second];
    String url = route.pattern;
    
    // Replace parameters
    for (const auto& param : params) {
        url.replace("<" + param.first + ">", param.second);
        url.replace("<" + param.first + ":int>", param.second);
        url.replace("<" + param.first + ":float>", param.second);
        url.replace("<" + param.first + ":uuid>", param.second);
    }
    
    return url;
}

bool Router::validateParam(const String& value, ParamType type) {
    switch (type) {
        case ParamType::INT: {
            for (size_t i = 0; i < value.length(); i++) {
                char c = value.charAt(i);
                if (i == 0 && c == '-') continue;
                if (c < '0' || c > '9') return false;
            }
            return true;
        }
        case ParamType::FLOAT: {
            bool hasDot = false;
            for (size_t i = 0; i < value.length(); i++) {
                char c = value.charAt(i);
                if (i == 0 && c == '-') continue;
                if (c == '.' && !hasDot) { hasDot = true; continue; }
                if (c < '0' || c > '9') return false;
            }
            return true;
        }
        case ParamType::UUID: {
            // Simple UUID validation (8-4-4-4-12)
            if (value.length() != 36) return false;
            for (size_t i = 0; i < value.length(); i++) {
                char c = value.charAt(i);
                if (i == 8 || i == 13 || i == 18 || i == 23) {
                    if (c != '-') return false;
                } else {
                    if (!isxdigit(c)) return false;
                }
            }
            return true;
        }
        case ParamType::STRING:
        default:
            return true;
    }
}

void Router::clear() {
    routes_.clear();
    staticDirs_.clear();
    namedRoutes_.clear();
    root_ = std::make_unique<TrieNode>();
}

} // namespace espweb
