/**
 * @file template.cpp
 * @brief ESP32 Web Framework - Template Engine Implementation
 * 
 * @author ESP32 Web Framework
 * @version 1.0.0
 */

#include "template.hpp"
#include "views.hpp"
#include "fileio.hpp"
#include "logger.hpp"

namespace espweb {

// Singleton instance
TemplateEngine& TemplateEngine::getInstance() {
    static TemplateEngine instance;
    return instance;
}

TemplateEngine::TemplateEngine() {
    mutex_ = xSemaphoreCreateMutex();
    
    // Register default filters
    filters_["upper"] = filters::upper;
    filters_["lower"] = filters::lower;
    filters_["capitalize"] = filters::capitalize;
    filters_["trim"] = filters::trim;
    filters_["escape"] = filters::escape;
    filters_["urlencode"] = filters::urlencode;
}

bool TemplateEngine::init(const String& templateDir) {
    templateDir_ = templateDir;
    LOG_INFO("Template", "Template engine initialized with dir: " + templateDir);
    return true;
}

String TemplateEngine::render(const String& templateName, const TemplateContext& context) {
    xSemaphoreTake(mutex_, portMAX_DELAY);
    
    // Check cache
    if (settings::TEMPLATE_CACHE_ENABLED) {
        auto it = cache_.find(templateName);
        if (it != cache_.end()) {
            cacheHits_++;
            String result = renderNode(it->second->root.get(), context);
            xSemaphoreGive(mutex_);
            return result;
        }
    }
    
    cacheMisses_++;
    
    // Compile template
    CompiledTemplate* compiled = compile(templateName);
    if (!compiled) {
        xSemaphoreGive(mutex_);
        return "";
    }
    
    String result = renderNode(compiled->root.get(), context);
    xSemaphoreGive(mutex_);
    return result;
}

String TemplateEngine::render(const String& templateName, 
                              const std::map<String, String>& context) {
    TemplateContext ctx;
    for (const auto& item : context) {
        ctx.set(item.first, item.second);
    }
    return render(templateName, ctx);
}

String TemplateEngine::renderString(const String& templateStr, const TemplateContext& context) {
    auto compiled = compileString(templateStr);
    if (!compiled) {
        return "";
    }
    return renderNode(compiled->root.get(), context);
}

CompiledTemplate* TemplateEngine::compile(const String& templateName) {
    // Load template content
    String content = loadTemplate(templateName);
    if (content.isEmpty()) {
        LOG_ERROR("Template", "Failed to load template: " + templateName);
        return nullptr;
    }
    
    // Compile
    auto compiled = compileString(content);
    if (!compiled) {
        return nullptr;
    }
    
    compiled->name = templateName;
    compiled->size = content.length();
    
    // Store in cache
    if (settings::TEMPLATE_CACHE_ENABLED) {
        cache_[templateName] = std::move(compiled);
        return cache_[templateName].get();
    }
    
    return compiled.get();
}

std::unique_ptr<CompiledTemplate> TemplateEngine::compileString(const String& templateStr) {
    auto compiled = std::make_unique<CompiledTemplate>();
    compiled->compiledAt = millis();
    
    // Tokenize
    std::vector<String> tokens = tokenize(templateStr);
    
    // Parse into AST
    size_t pos = 0;
    compiled->root = parse(tokens, pos);
    
    return compiled;
}

String TemplateEngine::loadTemplate(const String& templateName) {
    String path = templateDir_ + "/" + templateName;
    return Files().readFile(path);
}

std::vector<String> TemplateEngine::tokenize(const String& templateStr) {
    std::vector<String> tokens;
    String current;
    size_t i = 0;
    
    while (i < templateStr.length()) {
        // Check for variable tag {{ }}
        if (i + 1 < templateStr.length() && 
            templateStr.charAt(i) == '{' && templateStr.charAt(i + 1) == '{') {
            
            // Add text token if any
            if (!current.isEmpty()) {
                tokens.push_back("TEXT:" + current);
                current = "";
            }
            
            // Find closing }}
            i += 2;
            String varExpr;
            while (i + 1 < templateStr.length() && 
                   !(templateStr.charAt(i) == '}' && templateStr.charAt(i + 1) == '}')) {
                varExpr += templateStr.charAt(i);
                i++;
            }
            i += 2; // Skip }}
            
            varExpr.trim();
            tokens.push_back("VAR:" + varExpr);
        }
        // Check for block tag {% %}
        else if (i + 1 < templateStr.length() && 
                 templateStr.charAt(i) == '{' && templateStr.charAt(i + 1) == '%') {
            
            // Add text token if any
            if (!current.isEmpty()) {
                tokens.push_back("TEXT:" + current);
                current = "";
            }
            
            // Find closing %}
            i += 2;
            String blockExpr;
            while (i + 1 < templateStr.length() && 
                   !(templateStr.charAt(i) == '%' && templateStr.charAt(i + 1) == '}')) {
                blockExpr += templateStr.charAt(i);
                i++;
            }
            i += 2; // Skip %}
            
            blockExpr.trim();
            tokens.push_back("BLOCK:" + blockExpr);
        }
        // Check for comment {# #}
        else if (i + 1 < templateStr.length() && 
                 templateStr.charAt(i) == '{' && templateStr.charAt(i + 1) == '#') {
            
            // Add text token if any
            if (!current.isEmpty()) {
                tokens.push_back("TEXT:" + current);
                current = "";
            }
            
            // Find closing #}
            i += 2;
            while (i + 1 < templateStr.length() && 
                   !(templateStr.charAt(i) == '#' && templateStr.charAt(i + 1) == '}')) {
                i++;
            }
            i += 2; // Skip #}
            
            // Comments are ignored
        }
        else {
            current += templateStr.charAt(i);
            i++;
        }
    }
    
    if (!current.isEmpty()) {
        tokens.push_back("TEXT:" + current);
    }
    
    return tokens;
}

std::unique_ptr<TemplateNode> TemplateEngine::parse(const std::vector<String>& tokens, size_t& pos) {
    auto root = std::make_unique<TemplateNode>(NodeType::ROOT);
    
    while (pos < tokens.size()) {
        const String& token = tokens[pos];
        
        if (token.startsWith("TEXT:")) {
            auto node = std::make_unique<TemplateNode>(NodeType::TEXT);
            node->content = token.substring(5);
            root->children.push_back(std::move(node));
            pos++;
        }
        else if (token.startsWith("VAR:")) {
            auto node = std::make_unique<TemplateNode>(NodeType::VARIABLE);
            node->content = token.substring(4);
            root->children.push_back(std::move(node));
            pos++;
        }
        else if (token.startsWith("BLOCK:")) {
            String blockExpr = token.substring(6);
            
            if (blockExpr.startsWith("if ")) {
                auto node = std::make_unique<TemplateNode>(NodeType::IF);
                node->content = blockExpr.substring(3);
                pos++;
                
                // Parse if body
                auto ifBody = parse(tokens, pos);
                node->children = std::move(ifBody->children);
                
                // Check for else/elif
                while (pos < tokens.size()) {
                    const String& nextToken = tokens[pos];
                    if (nextToken.startsWith("BLOCK:")) {
                        String nextBlock = nextToken.substring(6);
                        if (nextBlock == "endif") {
                            pos++;
                            break;
                        } else if (nextBlock == "else") {
                            pos++;
                            node->elseBlock = parse(tokens, pos);
                        } else if (nextBlock.startsWith("elif ")) {
                            // Handle elif as nested if in else
                            break;
                        } else {
                            break;
                        }
                    } else {
                        break;
                    }
                }
                
                root->children.push_back(std::move(node));
            }
            else if (blockExpr.startsWith("for ")) {
                auto node = std::make_unique<TemplateNode>(NodeType::FOR);
                
                // Parse "for item in list"
                String forExpr = blockExpr.substring(4);
                int inPos = forExpr.indexOf(" in ");
                if (inPos > 0) {
                    node->varName = forExpr.substring(0, inPos);
                    node->varName.trim();
                    node->iterName = forExpr.substring(inPos + 4);
                    node->iterName.trim();
                }
                
                pos++;
                
                // Parse for body
                auto forBody = parse(tokens, pos);
                node->children = std::move(forBody->children);
                
                // Find endfor
                while (pos < tokens.size()) {
                    const String& nextToken = tokens[pos];
                    if (nextToken == "BLOCK:endfor") {
                        pos++;
                        break;
                    } else if (nextToken.startsWith("BLOCK:else")) {
                        pos++;
                        node->elseBlock = parse(tokens, pos);
                    } else {
                        break;
                    }
                }
                
                root->children.push_back(std::move(node));
            }
            else if (blockExpr.startsWith("include ")) {
                auto node = std::make_unique<TemplateNode>(NodeType::INCLUDE);
                String includeName = blockExpr.substring(8);
                includeName.trim();
                // Remove quotes
                if (includeName.startsWith("\"") || includeName.startsWith("'")) {
                    includeName = includeName.substring(1, includeName.length() - 1);
                }
                node->content = includeName;
                root->children.push_back(std::move(node));
                pos++;
            }
            else if (blockExpr == "endif" || blockExpr == "endfor" || 
                     blockExpr == "else" || blockExpr.startsWith("elif ")) {
                // End of block - return to parent
                break;
            }
            else {
                pos++;
            }
        }
        else {
            pos++;
        }
    }
    
    return root;
}

String TemplateEngine::renderNode(const TemplateNode* node, const TemplateContext& context) {
    if (!node) return "";
    
    String result;
    
    switch (node->type) {
        case NodeType::ROOT:
            for (const auto& child : node->children) {
                result += renderNode(child.get(), context);
            }
            break;
            
        case NodeType::TEXT:
            result = node->content;
            break;
            
        case NodeType::VARIABLE: {
            String varExpr = node->content;
            
            // Check for filters (var|filter)
            int pipePos = varExpr.indexOf('|');
            String varName = pipePos > 0 ? varExpr.substring(0, pipePos) : varExpr;
            varName.trim();
            
            String value = getVariable(varName, context);
            
            // Apply filters
            if (pipePos > 0) {
                String filterName = varExpr.substring(pipePos + 1);
                filterName.trim();
                value = applyFilter(value, filterName);
            }
            
            result = value;
            break;
        }
            
        case NodeType::IF: {
            bool condition = evaluateCondition(node->content, context);
            
            if (condition) {
                for (const auto& child : node->children) {
                    result += renderNode(child.get(), context);
                }
            } else if (node->elseBlock) {
                result = renderNode(node->elseBlock.get(), context);
            }
            break;
        }
            
        case NodeType::FOR: {
            // Get the iterable from context
            const JsonDocument& doc = context.getDocument();
            
            JsonVariantConst variant = doc[node->iterName];
            if (variant.is<JsonArrayConst>()) {
                JsonArrayConst arr = variant.as<JsonArrayConst>();
                
                if (arr.size() == 0 && node->elseBlock) {
                    result = renderNode(node->elseBlock.get(), context);
                } else {
                    for (JsonVariantConst item : arr) {
                        // Create new context with loop variable
                        TemplateContext loopContext;
                        // Copy parent context
                        JsonDocument& loopDoc = loopContext.getDocument();
                        loopDoc.set(doc);
                        
                        // Set loop variable
                        if (item.is<JsonObjectConst>()) {
                            loopDoc[node->varName].set(item);
                        } else {
                            loopDoc[node->varName] = item.as<String>();
                        }
                        
                        for (const auto& child : node->children) {
                            result += renderNode(child.get(), loopContext);
                        }
                    }
                }
            }
            break;
        }
            
        case NodeType::INCLUDE: {
            // Recursively render included template
            result = render(node->content, context);
            break;
        }
            
        default:
            break;
    }
    
    return result;
}

bool TemplateEngine::evaluateCondition(const String& expr, const TemplateContext& context) {
    String trimmedExpr = expr;
    trimmedExpr.trim();
    
    // Handle "not" prefix
    bool negate = false;
    if (trimmedExpr.startsWith("not ")) {
        negate = true;
        trimmedExpr = trimmedExpr.substring(4);
        trimmedExpr.trim();
    }
    
    // Handle comparisons
    int opPos = -1;
    String op;
    
    if ((opPos = trimmedExpr.indexOf("==")) > 0) {
        op = "==";
    } else if ((opPos = trimmedExpr.indexOf("!=")) > 0) {
        op = "!=";
    } else if ((opPos = trimmedExpr.indexOf(">=")) > 0) {
        op = ">=";
    } else if ((opPos = trimmedExpr.indexOf("<=")) > 0) {
        op = "<=";
    } else if ((opPos = trimmedExpr.indexOf(">")) > 0) {
        op = ">";
    } else if ((opPos = trimmedExpr.indexOf("<")) > 0) {
        op = "<";
    }
    
    bool result;
    
    if (opPos > 0) {
        String leftVar = trimmedExpr.substring(0, opPos);
        String rightVal = trimmedExpr.substring(opPos + op.length());
        leftVar.trim();
        rightVal.trim();
        
        String leftValue = getVariable(leftVar, context);
        
        // Remove quotes from right value if present
        if (rightVal.startsWith("\"") || rightVal.startsWith("'")) {
            rightVal = rightVal.substring(1, rightVal.length() - 1);
        } else {
            rightVal = getVariable(rightVal, context);
        }
        
        if (op == "==") result = (leftValue == rightVal);
        else if (op == "!=") result = (leftValue != rightVal);
        else if (op == ">") result = (leftValue.toFloat() > rightVal.toFloat());
        else if (op == "<") result = (leftValue.toFloat() < rightVal.toFloat());
        else if (op == ">=") result = (leftValue.toFloat() >= rightVal.toFloat());
        else if (op == "<=") result = (leftValue.toFloat() <= rightVal.toFloat());
        else result = false;
    } else {
        // Simple truthiness check
        String value = getVariable(trimmedExpr, context);
        result = !value.isEmpty() && value != "0" && value != "false";
    }
    
    return negate ? !result : result;
}

String TemplateEngine::getVariable(const String& varName, const TemplateContext& context) {
    const JsonDocument& doc = context.getDocument();
    
    // Handle dot notation (object.property)
    int dotPos = varName.indexOf('.');
    if (dotPos > 0) {
        String objName = varName.substring(0, dotPos);
        String propName = varName.substring(dotPos + 1);
        
        JsonVariantConst variant = doc[objName];
        if (variant.is<JsonObjectConst>()) {
            JsonObjectConst obj = variant.as<JsonObjectConst>();
            if (obj[propName].is<const char*>()) {
                return String(obj[propName].as<const char*>());
            } else if (obj[propName].is<int>()) {
                return String(obj[propName].as<int>());
            } else if (obj[propName].is<float>()) {
                return String(obj[propName].as<float>());
            } else if (obj[propName].is<bool>()) {
                return obj[propName].as<bool>() ? "true" : "false";
            }
        }
        return "";
    }
    
    // Simple variable
    return context.get(varName);
}

String TemplateEngine::applyFilter(const String& value, const String& filterName) {
    auto it = filters_.find(filterName);
    if (it != filters_.end()) {
        return it->second(value);
    }
    return value;
}

bool TemplateEngine::preload(const String& templateName) {
    xSemaphoreTake(mutex_, portMAX_DELAY);
    CompiledTemplate* compiled = compile(templateName);
    xSemaphoreGive(mutex_);
    return compiled != nullptr;
}

void TemplateEngine::clearCache() {
    xSemaphoreTake(mutex_, portMAX_DELAY);
    cache_.clear();
    xSemaphoreGive(mutex_);
}

void TemplateEngine::invalidate(const String& templateName) {
    xSemaphoreTake(mutex_, portMAX_DELAY);
    cache_.erase(templateName);
    xSemaphoreGive(mutex_);
}

void TemplateEngine::registerFilter(const String& name, TemplateFilter filter) {
    filters_[name] = filter;
}

void TemplateEngine::getCacheStats(uint32_t& hits, uint32_t& misses) const {
    hits = cacheHits_;
    misses = cacheMisses_;
}

//==============================================================================
// Built-in Filters Implementation
//==============================================================================

namespace filters {

String upper(const String& value) {
    String result = value;
    result.toUpperCase();
    return result;
}

String lower(const String& value) {
    String result = value;
    result.toLowerCase();
    return result;
}

String capitalize(const String& value) {
    if (value.isEmpty()) return value;
    String result = value;
    result.toLowerCase();
    result.setCharAt(0, toupper(result.charAt(0)));
    return result;
}

String trim(const String& value) {
    String result = value;
    result.trim();
    return result;
}

String escape(const String& value) {
    String result;
    for (size_t i = 0; i < value.length(); i++) {
        char c = value.charAt(i);
        switch (c) {
            case '&': result += "&amp;"; break;
            case '<': result += "&lt;"; break;
            case '>': result += "&gt;"; break;
            case '"': result += "&quot;"; break;
            case '\'': result += "&#39;"; break;
            default: result += c;
        }
    }
    return result;
}

String truncate(const String& value, size_t length) {
    if (value.length() <= length) return value;
    return value.substring(0, length) + "...";
}

String urlencode(const String& value) {
    String result;
    for (size_t i = 0; i < value.length(); i++) {
        char c = value.charAt(i);
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            result += c;
        } else if (c == ' ') {
            result += '+';
        } else {
            char buf[4];
            sprintf(buf, "%%%02X", (unsigned char)c);
            result += buf;
        }
    }
    return result;
}

} // namespace filters

} // namespace espweb
