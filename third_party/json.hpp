// Simplified JSON library - single header implementation
#pragma once

#include <string>
#include <vector>
#include <map>
#include <variant>
#include <sstream>
#include <stdexcept>

namespace nlohmann {

class json {
public:
    using value_t = std::variant<
        std::nullptr_t,
        bool,
        int64_t,
        double,
        std::string,
        std::vector<json>,
        std::map<std::string, json>
    >;
    
    value_t data;
    
    json() : data(nullptr) {}
    json(std::nullptr_t) : data(nullptr) {}
    json(bool val) : data(val) {}
    json(int val) : data(static_cast<int64_t>(val)) {}
    json(int64_t val) : data(val) {}
    json(double val) : data(val) {}
    json(const std::string& val) : data(val) {}
    json(const char* val) : data(std::string(val)) {}
    json(const std::vector<json>& val) : data(val) {}
    json(const std::map<std::string, json>& val) : data(val) {}
    
    bool is_null() const { return std::holds_alternative<std::nullptr_t>(data); }
    bool is_boolean() const { return std::holds_alternative<bool>(data); }
    bool is_number_integer() const { return std::holds_alternative<int64_t>(data); }
    bool is_number_float() const { return std::holds_alternative<double>(data); }
    bool is_string() const { return std::holds_alternative<std::string>(data); }
    bool is_array() const { return std::holds_alternative<std::vector<json>>(data); }
    bool is_object() const { return std::holds_alternative<std::map<std::string, json>>(data); }
    
    bool get_bool() const {
        if (is_boolean()) return std::get<bool>(data);
        throw std::runtime_error("Not a boolean");
    }
    
    int64_t get_int() const {
        if (is_number_integer()) return std::get<int64_t>(data);
        throw std::runtime_error("Not an integer");
    }
    
    double get_double() const {
        if (is_number_float()) return std::get<double>(data);
        if (is_number_integer()) return static_cast<double>(std::get<int64_t>(data));
        throw std::runtime_error("Not a number");
    }
    
    std::string get_string() const {
        if (is_string()) return std::get<std::string>(data);
        throw std::runtime_error("Not a string");
    }
    
    std::vector<json> get_array() const {
        if (is_array()) return std::get<std::vector<json>>(data);
        throw std::runtime_error("Not an array");
    }
    
    std::map<std::string, json> get_object() const {
        if (is_object()) return std::get<std::map<std::string, json>>(data);
        throw std::runtime_error("Not an object");
    }
    
    json& operator[](const std::string& key) {
        if (!is_object()) {
            data = std::map<std::string, json>{};
        }
        return std::get<std::map<std::string, json>>(data)[key];
    }
    
    const json& operator[](const std::string& key) const {
        if (!is_object()) {
            throw std::runtime_error("Not an object");
        }
        auto& obj = std::get<std::map<std::string, json>>(data);
        auto it = obj.find(key);
        if (it == obj.end()) {
            static const json null_json;
            return null_json;
        }
        return it->second;
    }
    
    json& operator[](size_t index) {
        if (!is_array()) {
            throw std::runtime_error("Not an array");
        }
        auto& arr = std::get<std::vector<json>>(data);
        if (index >= arr.size()) {
            arr.resize(index + 1);
        }
        return arr[index];
    }
    
    const json& operator[](size_t index) const {
        if (!is_array()) {
            throw std::runtime_error("Not an array");
        }
        const auto& arr = std::get<std::vector<json>>(data);
        if (index >= arr.size()) {
            static const json null_json;
            return null_json;
        }
        return arr[index];
    }
    
    size_t size() const {
        if (is_array()) return std::get<std::vector<json>>(data).size();
        if (is_object()) return std::get<std::map<std::string, json>>(data).size();
        return 1;
    }
    
    std::string dump(int indent = -1) const {
        std::ostringstream oss;
        dump_impl(oss, 0, indent < 0 ? -1 : indent);
        return oss.str();
    }
    
private:
    void dump_impl(std::ostringstream& oss, int current_indent, int max_indent) const {
        if (is_null()) {
            oss << "null";
        } else if (is_boolean()) {
            oss << (get_bool() ? "true" : "false");
        } else if (is_number_integer()) {
            oss << get_int();
        } else if (is_number_float()) {
            oss << get_double();
        } else if (is_string()) {
            oss << "\"" << escape_string(get_string()) << "\"";
        } else if (is_array()) {
            const auto& arr = get_array();
            oss << "[";
            if (max_indent >= 0 && current_indent < max_indent) {
                oss << "\n" << std::string(current_indent + 1, ' ');
            }
            for (size_t i = 0; i < arr.size(); ++i) {
                if (i > 0) {
                    oss << ",";
                    if (max_indent >= 0 && current_indent < max_indent) {
                        oss << "\n" << std::string(current_indent + 1, ' ');
                    } else {
                        oss << " ";
                    }
                }
                arr[i].dump_impl(oss, current_indent + 1, max_indent);
            }
            if (max_indent >= 0 && current_indent < max_indent) {
                oss << "\n" << std::string(current_indent, ' ');
            }
            oss << "]";
        } else if (is_object()) {
            const auto& obj = get_object();
            oss << "{";
            if (max_indent >= 0 && current_indent < max_indent) {
                oss << "\n" << std::string(current_indent + 1, ' ');
            }
            bool first = true;
            for (const auto& [key, value] : obj) {
                if (!first) {
                    oss << ",";
                    if (max_indent >= 0 && current_indent < max_indent) {
                        oss << "\n" << std::string(current_indent + 1, ' ');
                    } else {
                        oss << " ";
                    }
                }
                first = false;
                oss << "\"" << escape_string(key) << "\":";
                if (max_indent >= 0 && current_indent < max_indent) {
                    oss << " ";
                }
                value.dump_impl(oss, current_indent + 1, max_indent);
            }
            if (max_indent >= 0 && current_indent < max_indent) {
                oss << "\n" << std::string(current_indent, ' ');
            }
            oss << "}";
        }
    }
    
    std::string escape_string(const std::string& str) const {
        std::string result;
        for (char c : str) {
            switch (c) {
                case '"': result += "\\\""; break;
                case '\\': result += "\\\\"; break;
                case '\b': result += "\\b"; break;
                case '\f': result += "\\f"; break;
                case '\n': result += "\\n"; break;
                case '\r': result += "\\r"; break;
                case '\t': result += "\\t"; break;
                default: result += c; break;
            }
        }
        return result;
    }
};

// Parse functions
json parse(const std::string& str) {
    // Simplified parser - for production use, consider a proper JSON parser
    std::istringstream iss(str);
    return parse_impl(iss);
}

json parse(std::istringstream& iss) {
    return parse_impl(iss);
}

private:
json parse_impl(std::istringstream& iss) {
    char c;
    iss >> c;
    
    if (c == 'n') {
        // null
        iss.seekg(-1, std::ios::cur);
        std::string word;
        iss >> word;
        if (word == "null") return json(nullptr);
        throw std::runtime_error("Invalid JSON");
    } else if (c == 't' || c == 'f') {
        // boolean
        iss.seekg(-1, std::ios::cur);
        std::string word;
        iss >> word;
        if (word == "true") return json(true);
        if (word == "false") return json(false);
        throw std::runtime_error("Invalid JSON");
    } else if (c == '"') {
        // string
        std::string str;
        while (iss.get(c) && c != '"') {
            if (c == '\\') {
                iss.get(c);
                switch (c) {
                    case '"': str += '"'; break;
                    case '\\': str += '\\'; break;
                    case 'b': str += '\b'; break;
                    case 'f': str += '\f'; break;
                    case 'n': str += '\n'; break;
                    case 'r': str += '\r'; break;
                    case 't': str += '\t'; break;
                    default: str += c; break;
                }
            } else {
                str += c;
            }
        }
        return json(str);
    } else if (c == '[') {
        // array
        std::vector<json> arr;
        iss >> std::ws;
        if (iss.peek() == ']') {
            iss.get();
            return json(arr);
        }
        while (true) {
            arr.push_back(parse_impl(iss));
            iss >> std::ws;
            c = iss.peek();
            if (c == ']') {
                iss.get();
                break;
            } else if (c == ',') {
                iss.get();
                iss >> std::ws;
            } else {
                throw std::runtime_error("Invalid JSON array");
            }
        }
        return json(arr);
    } else if (c == '{') {
        // object
        std::map<std::string, json> obj;
        iss >> std::ws;
        if (iss.peek() == '}') {
            iss.get();
            return json(obj);
        }
        while (true) {
            iss >> std::ws;
            std::string key = parse_impl(iss).get_string();
            iss >> std::ws;
            if (iss.get() != ':') {
                throw std::runtime_error("Expected ':' in JSON object");
            }
            iss >> std::ws;
            json value = parse_impl(iss);
            obj[key] = value;
            iss >> std::ws;
            c = iss.peek();
            if (c == '}') {
                iss.get();
                break;
            } else if (c == ',') {
                iss.get();
                iss >> std::ws;
            } else {
                throw std::runtime_error("Invalid JSON object");
            }
        }
        return json(obj);
    } else if (std::isdigit(c) || c == '-') {
        // number
        iss.seekg(-1, std::ios::cur);
        std::string num_str;
        bool is_float = false;
        while (iss.get(c) && (std::isdigit(c) || c == '.' || c == 'e' || c == 'E' || c == '+' || c == '-')) {
            if (c == '.' || c == 'e' || c == 'E') {
                is_float = true;
            }
            num_str += c;
        }
        iss.seekg(-1, std::ios::cur);
        
        if (is_float) {
            return json(std::stod(num_str));
        } else {
            return json(std::stoll(num_str));
        }
    }
    
    throw std::runtime_error("Invalid JSON");
}

} // namespace nlohmann

