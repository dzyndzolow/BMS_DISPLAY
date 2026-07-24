/**
 * @file views.hpp
 * @brief ESP32 Web Framework - View Classes
 * 
 * Base classes for views:
 * - View (base)
 * - TemplateView
 * - JsonView
 * - FileView
 * - FormView
 * 
 * @author ESP32 Web Framework
 * @version 1.0.0
 */

#ifndef ESP_WEB_FRAMEWORK_VIEWS_HPP
#define ESP_WEB_FRAMEWORK_VIEWS_HPP

#include <Arduino.h>
#include <map>
#include <functional>
#include <ArduinoJson.h>

#include "http.hpp"

namespace espweb {

// Forward declaration
class TemplateEngine;

/**
 * @brief Template context for variable substitution
 */
using Context = std::map<String, String>;

/**
 * @brief Extended context with JSON support
 */
class TemplateContext {
public:
    TemplateContext() = default;
    ~TemplateContext() = default;
    
    /**
     * @brief Set string variable
     */
    void set(const String& key, const String& value);
    
    /**
     * @brief Set integer variable
     */
    void set(const String& key, int value);
    
    /**
     * @brief Set float variable
     */
    void set(const String& key, float value);
    
    /**
     * @brief Set boolean variable
     */
    void set(const String& key, bool value);
    
    /**
     * @brief Set JSON array for loops
     */
    void setArray(const String& key, const JsonArray& arr);
    
    /**
     * @brief Set JSON object
     */
    void setObject(const String& key, const JsonObject& obj);
    
    /**
     * @brief Get value as string
     */
    String get(const String& key) const;
    
    /**
     * @brief Check if key exists
     */
    bool has(const String& key) const;
    
    /**
     * @brief Get internal document
     */
    JsonDocument& getDocument() { return doc_; }
    const JsonDocument& getDocument() const { return doc_; }
    
private:
    JsonDocument doc_;
};

/**
 * @brief Base View class
 * 
 * All views should inherit from this class.
 */
class View {
public:
    virtual ~View() = default;
    
    /**
     * @brief Handle request
     * @param request Request object
     * @return Response object
     */
    virtual Response handle(Request& request) = 0;
    
    /**
     * @brief Get allowed methods
     * @return Vector of allowed HTTP methods
     */
    virtual std::vector<HttpMethod> getAllowedMethods() const {
        return { HttpMethod::GET };
    }
};

/**
 * @brief Template-based view
 * 
 * Renders HTML templates with context variables.
 */
class TemplateView : public View {
public:
    TemplateView() = default;
    explicit TemplateView(const String& templateName);
    virtual ~TemplateView() = default;
    
    Response handle(Request& request) override;
    
    /**
     * @brief Get template name
     * @return Template filename
     */
    virtual String getTemplateName() const { return templateName_; }
    
    /**
     * @brief Get context for template
     * @param request Request object
     * @return Template context
     */
    virtual TemplateContext getContext(Request& request);
    
protected:
    /**
     * @brief Render template with context
     * @param templateName Template filename
     * @param context Template context
     * @return Response with rendered HTML
     */
    Response render(const String& templateName, const TemplateContext& context);
    
    /**
     * @brief Render template with simple context
     * @param templateName Template filename
     * @param context Simple string context
     * @return Response with rendered HTML
     */
    Response render(const String& templateName, const Context& context);
    
    String templateName_;
};

/**
 * @brief JSON API view
 * 
 * Returns JSON responses.
 */
class JsonView : public View {
public:
    virtual ~JsonView() = default;
    
    Response handle(Request& request) override;
    
    /**
     * @brief Get JSON data
     * @param request Request object
     * @param doc JSON document to populate
     * @return HTTP status code
     */
    virtual HttpStatus getData(Request& request, JsonDocument& doc) = 0;
    
    std::vector<HttpMethod> getAllowedMethods() const override {
        return { HttpMethod::GET, HttpMethod::POST };
    }
};

/**
 * @brief File download view
 * 
 * Serves files for download.
 */
class FileView : public View {
public:
    FileView() = default;
    explicit FileView(const String& filePath);
    virtual ~FileView() = default;
    
    Response handle(Request& request) override;
    
    /**
     * @brief Get file path
     * @param request Request object
     * @return File path
     */
    virtual String getFilePath(Request& request) const { return filePath_; }
    
    /**
     * @brief Get download filename
     * @param request Request object
     * @return Filename for download
     */
    virtual String getFilename(Request& request) const { return ""; }
    
protected:
    String filePath_;
};

/**
 * @brief Form handling view
 * 
 * Handles form submissions with validation.
 */
class FormView : public View {
public:
    virtual ~FormView() = default;
    
    Response handle(Request& request) override;
    
    std::vector<HttpMethod> getAllowedMethods() const override {
        return { HttpMethod::GET, HttpMethod::POST };
    }
    
protected:
    /**
     * @brief Handle GET request (show form)
     * @param request Request object
     * @return Response with form HTML
     */
    virtual Response get(Request& request) = 0;
    
    /**
     * @brief Handle POST request (process form)
     * @param request Request object
     * @return Response after processing
     */
    virtual Response post(Request& request) = 0;
    
    /**
     * @brief Validate form data
     * @param request Request object
     * @param errors Output error messages
     * @return true if valid
     */
    virtual bool validate(Request& request, std::map<String, String>& errors) {
        return true;
    }
};

/**
 * @brief Redirect view
 * 
 * Simple redirect to another URL.
 */
class RedirectView : public View {
public:
    RedirectView(const String& url, bool permanent = false);
    virtual ~RedirectView() = default;
    
    Response handle(Request& request) override;
    
protected:
    String url_;
    bool permanent_;
};

/**
 * @brief Static content view
 * 
 * Returns static content defined in code.
 */
class StaticContentView : public View {
public:
    StaticContentView(const String& content, const String& contentType = "text/html");
    virtual ~StaticContentView() = default;
    
    Response handle(Request& request) override;
    
protected:
    String content_;
    String contentType_;
};

//==============================================================================
// View Factory Helpers
//==============================================================================

/**
 * @brief Create view handler from lambda
 */
inline ViewHandler makeView(std::function<Response(Request&)> handler) {
    return handler;
}

/**
 * @brief Create template view handler
 */
ViewHandler makeTemplateView(const String& templateName, 
                             std::function<TemplateContext(Request&)> contextFunc = nullptr);

/**
 * @brief Create JSON view handler
 */
ViewHandler makeJsonView(std::function<void(Request&, JsonDocument&)> dataFunc);

/**
 * @brief Create redirect view handler
 */
ViewHandler makeRedirectView(const String& url, bool permanent = false);

/**
 * @brief Create static content view handler
 */
ViewHandler makeStaticView(const String& content, const String& contentType = "text/html");

} // namespace espweb

#endif // ESP_WEB_FRAMEWORK_VIEWS_HPP
