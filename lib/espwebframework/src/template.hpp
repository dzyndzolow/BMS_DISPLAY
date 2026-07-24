/**
 * @file template.hpp
 * @brief ESP32 Web Framework - Template Engine
 * 
 * Django/Jinja2-like template engine with:
 * - {{ variable }} substitution
 * - {% if condition %} conditionals
 * - {% for item in list %} loops
 * - {% include "file" %} includes
 * - AST-based compilation and rendering
 * 
 * @author ESP32 Web Framework
 * @version 1.0.0
 */

#ifndef ESP_WEB_FRAMEWORK_TEMPLATE_HPP
#define ESP_WEB_FRAMEWORK_TEMPLATE_HPP

#include <Arduino.h>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <ArduinoJson.h>

#include "settings.h"

namespace espweb {

// Forward declaration
class TemplateContext;

/**
 * @brief Template AST node types
 */
enum class NodeType {
    TEXT,           ///< Static text
    VARIABLE,       ///< {{ variable }}
    IF,             ///< {% if condition %}
    ELIF,           ///< {% elif condition %}
    ELSE,           ///< {% else %}
    ENDIF,          ///< {% endif %}
    FOR,            ///< {% for item in list %}
    ENDFOR,         ///< {% endfor %}
    INCLUDE,        ///< {% include "file" %}
    BLOCK,          ///< {% block name %}
    ENDBLOCK,       ///< {% endblock %}
    EXTENDS,        ///< {% extends "base" %}
    COMMENT,        ///< {# comment #}
    ROOT            ///< Root node
};

/**
 * @brief Template AST node
 */
struct TemplateNode {
    NodeType type = NodeType::TEXT;
    String content;                              ///< Text content or expression
    String varName;                              ///< Variable/loop variable name
    String iterName;                             ///< Iterator name for loops
    std::vector<std::unique_ptr<TemplateNode>> children;
    std::unique_ptr<TemplateNode> elseBlock;     ///< Else branch for if/for
    
    TemplateNode() = default;
    explicit TemplateNode(NodeType t) : type(t) {}
    TemplateNode(NodeType t, const String& c) : type(t), content(c) {}
};

/**
 * @brief Compiled template
 */
struct CompiledTemplate {
    String name;                                 ///< Template name
    std::unique_ptr<TemplateNode> root;          ///< AST root
    uint32_t compiledAt;                         ///< Compilation timestamp
    size_t size;                                 ///< Original template size
};

/**
 * @brief Template filter function type
 */
using TemplateFilter = std::function<String(const String&)>;

/**
 * @brief Template Engine class - Singleton
 * 
 * Compiles and renders templates with Django-like syntax.
 */
class TemplateEngine {
public:
    /**
     * @brief Get singleton instance
     */
    static TemplateEngine& getInstance();
    
    // Delete copy constructor and assignment
    TemplateEngine(const TemplateEngine&) = delete;
    TemplateEngine& operator=(const TemplateEngine&) = delete;
    
    /**
     * @brief Initialize template engine
     * @param templateDir Base directory for templates
     * @return true if successful
     */
    bool init(const String& templateDir = TEMPLATE_DIR);
    
    /**
     * @brief Render template with context
     * @param templateName Template filename
     * @param context Template context
     * @return Rendered HTML string
     */
    String render(const String& templateName, const TemplateContext& context);
    
    /**
     * @brief Render template with simple context
     * @param templateName Template filename
     * @param context Simple string map context
     * @return Rendered HTML string
     */
    String render(const String& templateName, const std::map<String, String>& context);
    
    /**
     * @brief Render template string directly
     * @param templateStr Template string
     * @param context Template context
     * @return Rendered HTML string
     */
    String renderString(const String& templateStr, const TemplateContext& context);
    
    /**
     * @brief Compile template
     * @param templateName Template filename
     * @return Pointer to compiled template or nullptr
     */
    CompiledTemplate* compile(const String& templateName);
    
    /**
     * @brief Compile template string
     * @param templateStr Template string
     * @return Unique pointer to compiled template
     */
    std::unique_ptr<CompiledTemplate> compileString(const String& templateStr);
    
    /**
     * @brief Preload and cache template
     * @param templateName Template filename
     * @return true if successful
     */
    bool preload(const String& templateName);
    
    /**
     * @brief Clear template cache
     */
    void clearCache();
    
    /**
     * @brief Remove template from cache
     * @param templateName Template filename
     */
    void invalidate(const String& templateName);
    
    /**
     * @brief Register custom filter
     * @param name Filter name
     * @param filter Filter function
     */
    void registerFilter(const String& name, TemplateFilter filter);
    
    /**
     * @brief Get cache statistics
     * @param hits Output hit count
     * @param misses Output miss count
     */
    void getCacheStats(uint32_t& hits, uint32_t& misses) const;
    
    /**
     * @brief Set template directory
     * @param dir Directory path
     */
    void setTemplateDir(const String& dir) { templateDir_ = dir; }
    
    /**
     * @brief Get template directory
     */
    const String& getTemplateDir() const { return templateDir_; }
    
private:
    TemplateEngine();
    ~TemplateEngine() = default;
    
    /**
     * @brief Load template from filesystem
     * @param templateName Template filename
     * @return Template content or empty string
     */
    String loadTemplate(const String& templateName);
    
    /**
     * @brief Tokenize template string
     * @param templateStr Template string
     * @return Vector of tokens
     */
    std::vector<String> tokenize(const String& templateStr);
    
    /**
     * @brief Parse tokens into AST
     * @param tokens Token vector
     * @param pos Current position
     * @return AST root node
     */
    std::unique_ptr<TemplateNode> parse(const std::vector<String>& tokens, size_t& pos);
    
    /**
     * @brief Render AST node
     * @param node AST node
     * @param context Template context
     * @return Rendered string
     */
    String renderNode(const TemplateNode* node, const TemplateContext& context);
    
    /**
     * @brief Evaluate condition expression
     * @param expr Expression string
     * @param context Template context
     * @return true if condition is true
     */
    bool evaluateCondition(const String& expr, const TemplateContext& context);
    
    /**
     * @brief Get variable value from context
     * @param varName Variable name (supports dot notation)
     * @param context Template context
     * @return Variable value as string
     */
    String getVariable(const String& varName, const TemplateContext& context);
    
    /**
     * @brief Apply filter to value
     * @param value Input value
     * @param filterName Filter name
     * @return Filtered value
     */
    String applyFilter(const String& value, const String& filterName);
    
    String templateDir_;
    std::map<String, std::unique_ptr<CompiledTemplate>> cache_;
    std::map<String, TemplateFilter> filters_;
    
    uint32_t cacheHits_ = 0;
    uint32_t cacheMisses_ = 0;
    
    SemaphoreHandle_t mutex_;
};

/**
 * @brief Convenience function to get template engine
 */
inline TemplateEngine& Templates() {
    return TemplateEngine::getInstance();
}

//==============================================================================
// Built-in Filters
//==============================================================================

namespace filters {

/** @brief Convert to uppercase */
String upper(const String& value);

/** @brief Convert to lowercase */
String lower(const String& value);

/** @brief Capitalize first letter */
String capitalize(const String& value);

/** @brief Trim whitespace */
String trim(const String& value);

/** @brief HTML escape */
String escape(const String& value);

/** @brief Truncate to length */
String truncate(const String& value, size_t length = 100);

/** @brief Format as date */
String date(const String& value, const String& format = "%Y-%m-%d");

/** @brief Default value if empty */
String defaultValue(const String& value, const String& def);

/** @brief Join array with separator */
String join(const JsonArray& arr, const String& separator = ", ");

/** @brief Get array length */
String length(const String& value);

/** @brief URL encode */
String urlencode(const String& value);

} // namespace filters

} // namespace espweb

#endif // ESP_WEB_FRAMEWORK_TEMPLATE_HPP
