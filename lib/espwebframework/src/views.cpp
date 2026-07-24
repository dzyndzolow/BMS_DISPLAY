/**
 * @file views.cpp
 * @brief ESP32 Web Framework - Views Implementation
 * 
 * @author ESP32 Web Framework
 * @version 1.0.0
 */

#include "views.hpp"
#include "template.hpp"

namespace espweb {

//==============================================================================
// TemplateContext Implementation
//==============================================================================

void TemplateContext::set(const String& key, const String& value) {
    doc_[key] = value;
}

void TemplateContext::set(const String& key, int value) {
    doc_[key] = value;
}

void TemplateContext::set(const String& key, float value) {
    doc_[key] = value;
}

void TemplateContext::set(const String& key, bool value) {
    doc_[key] = value;
}

void TemplateContext::setArray(const String& key, const JsonArray& arr) {
    doc_[key] = arr;
}

void TemplateContext::setObject(const String& key, const JsonObject& obj) {
    doc_[key] = obj;
}

String TemplateContext::get(const String& key) const {
    if (doc_[key].is<String>()) {
        return doc_[key].as<String>();
    } else if (doc_[key].is<int>()) {
        return String(doc_[key].as<int>());
    } else if (doc_[key].is<float>()) {
        return String(doc_[key].as<float>());
    } else if (doc_[key].is<bool>()) {
        return doc_[key].as<bool>() ? "true" : "false";
    }
    return "";
}

bool TemplateContext::has(const String& key) const {
    return doc_.containsKey(key);
}

//==============================================================================
// TemplateView Implementation
//==============================================================================

TemplateView::TemplateView(const String& templateName) 
    : templateName_(templateName) {
}

Response TemplateView::handle(Request& request) {
    TemplateContext context = getContext(request);
    return render(getTemplateName(), context);
}

TemplateContext TemplateView::getContext(Request& request) {
    return TemplateContext();
}

Response TemplateView::render(const String& templateName, const TemplateContext& context) {
    String html = Templates().render(templateName, context);
    
    if (html.isEmpty()) {
        return Response::error(HttpStatus::INTERNAL_SERVER_ERROR, 
                              "Template not found: " + templateName);
    }
    
    return Response::html(html);
}

Response TemplateView::render(const String& templateName, const Context& context) {
    std::map<String, String> ctx;
    for (const auto& item : context) {
        ctx[item.first] = item.second;
    }
    
    String html = Templates().render(templateName, ctx);
    
    if (html.isEmpty()) {
        return Response::error(HttpStatus::INTERNAL_SERVER_ERROR,
                              "Template not found: " + templateName);
    }
    
    return Response::html(html);
}

//==============================================================================
// JsonView Implementation
//==============================================================================

Response JsonView::handle(Request& request) {
    JsonDocument doc;
    HttpStatus status = getData(request, doc);
    
    Response response = Response::json(doc, status);
    return response;
}

//==============================================================================
// FileView Implementation
//==============================================================================

FileView::FileView(const String& filePath) 
    : filePath_(filePath) {
}

Response FileView::handle(Request& request) {
    String path = getFilePath(request);
    String filename = getFilename(request);
    
    return Response::file(path, filename);
}

//==============================================================================
// FormView Implementation
//==============================================================================

Response FormView::handle(Request& request) {
    if (request.method == HttpMethod::POST) {
        std::map<String, String> errors;
        
        if (validate(request, errors)) {
            return post(request);
        } else {
            // Return form with errors
            // This should be overridden in subclass
            return Response::error(HttpStatus::BAD_REQUEST, "Validation failed");
        }
    }
    
    return get(request);
}

//==============================================================================
// RedirectView Implementation
//==============================================================================

RedirectView::RedirectView(const String& url, bool permanent)
    : url_(url), permanent_(permanent) {
}

Response RedirectView::handle(Request& request) {
    return Response::redirect(url_, permanent_);
}

//==============================================================================
// StaticContentView Implementation
//==============================================================================

StaticContentView::StaticContentView(const String& content, const String& contentType)
    : content_(content), contentType_(contentType) {
}

Response StaticContentView::handle(Request& request) {
    Response response;
    response.body = content_;
    response.contentType = contentType_;
    return response;
}

//==============================================================================
// View Factory Helpers
//==============================================================================

ViewHandler makeTemplateView(const String& templateName,
                             std::function<TemplateContext(Request&)> contextFunc) {
    return [templateName, contextFunc](Request& request) -> Response {
        TemplateContext context;
        
        if (contextFunc) {
            context = contextFunc(request);
        }
        
        String html = Templates().render(templateName, context);
        
        if (html.isEmpty()) {
            return Response::error(HttpStatus::INTERNAL_SERVER_ERROR,
                                  "Template not found: " + templateName);
        }
        
        return Response::html(html);
    };
}

ViewHandler makeJsonView(std::function<void(Request&, JsonDocument&)> dataFunc) {
    return [dataFunc](Request& request) -> Response {
        JsonDocument doc;
        dataFunc(request, doc);
        return Response::json(doc);
    };
}

ViewHandler makeRedirectView(const String& url, bool permanent) {
    return [url, permanent](Request& request) -> Response {
        return Response::redirect(url, permanent);
    };
}

ViewHandler makeStaticView(const String& content, const String& contentType) {
    return [content, contentType](Request& request) -> Response {
        Response response;
        response.body = content;
        response.contentType = contentType;
        return response;
    };
}

} // namespace espweb
