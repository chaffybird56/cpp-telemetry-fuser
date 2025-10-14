#pragma once

#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <stdexcept>

namespace nlohmann {

class json {
public:
    enum Type { null, boolean, number, string, array, object };
    
private:
    Type type_;
    union {
        bool bool_value;
        double number_value;
        std::string* string_value;
        std::vector<json>* array_value;
        std::map<std::string, json>* object_value;
    };
    
public:
    json() : type_(null) {}
    json(std::nullptr_t) : type_(null) {}
    json(bool val) : type_(boolean), bool_value(val) {}
    json(int val) : type_(number), number_value(val) {}
    json(double val) : type_(number), number_value(val) {}
    json(const std::string& val) : type_(string), string_value(new std::string(val)) {}
    json(const char* val) : type_(string), string_value(new std::string(val)) {}
    json(const std::vector<json>& val) : type_(array), array_value(new std::vector<json>(val)) {}
    json(const std::map<std::string, json>& val) : type_(object), object_value(new std::map<std::string, json>(val)) {}
    
    ~json() {
        cleanup();
    }
    
    json(const json& other) : type_(other.type_) {
        copy_from(other);
    }
    
    json& operator=(const json& other) {
        if (this != &other) {
            cleanup();
            type_ = other.type_;
            copy_from(other);
        }
        return *this;
    }
    
    bool is_null() const { return type_ == null; }
    bool is_boolean() const { return type_ == boolean; }
    bool is_number() const { return type_ == number; }
    bool is_string() const { return type_ == string; }
    bool is_array() const { return type_ == array; }
    bool is_object() const { return type_ == object; }
    
    bool get_bool() const {
        if (type_ != boolean) throw std::runtime_error("Not a boolean");
        return bool_value;
    }
    
    double get_double() const {
        if (type_ != number) throw std::runtime_error("Not a number");
        return number_value;
    }
    
    int get_int() const {
        return static_cast<int>(get_double());
    }
    
    std::string get_string() const {
        if (type_ != string) throw std::runtime_error("Not a string");
        return *string_value;
    }
    
    std::vector<json> get_array() const {
        if (type_ != array) throw std::runtime_error("Not an array");
        return *array_value;
    }
    
    std::map<std::string, json> get_object() const {
        if (type_ != object) throw std::runtime_error("Not an object");
        return *object_value;
    }
    
    json& operator[](const std::string& key) {
        if (type_ != object) {
            cleanup();
            type_ = object;
            object_value = new std::map<std::string, json>();
        }
        return (*object_value)[key];
    }
    
    const json& operator[](const std::string& key) const {
        if (type_ != object) {
            static const json null_json;
            return null_json;
        }
        auto it = object_value->find(key);
        if (it == object_value->end()) {
            static const json null_json;
            return null_json;
        }
        return it->second;
    }
    
    json& operator[](size_t index) {
        if (type_ != array) {
            cleanup();
            type_ = array;
            array_value = new std::vector<json>();
        }
        if (index >= array_value->size()) {
            array_value->resize(index + 1);
        }
        return (*array_value)[index];
    }
    
    const json& operator[](size_t index) const {
        if (type_ != array || index >= array_value->size()) {
            static const json null_json;
            return null_json;
        }
        return (*array_value)[index];
    }
    
    size_t size() const {
        switch (type_) {
            case array: return array_value->size();
            case object: return object_value->size();
            default: return 1;
        }
    }
    
    bool contains(const std::string& key) const {
        if (type_ != object) return false;
        return object_value->find(key) != object_value->end();
    }
    
    std::string dump(int indent = -1) const {
        std::ostringstream oss;
        dump_impl(oss, 0, indent < 0 ? -1 : indent);
        return oss.str();
    }
    
    static json parse(const std::string& str) {
        std::istringstream iss(str);
        return parse_impl(iss);
    }
    
private:
    void cleanup() {
        switch (type_) {
            case string: delete string_value; break;
            case array: delete array_value; break;
            case object: delete object_value; break;
            default: break;
        }
        type_ = null;
    }
    
    void copy_from(const json& other) {
        switch (other.type_) {
            case null: break;
            case boolean: bool_value = other.bool_value; break;
            case number: number_value = other.number_value; break;
            case string: string_value = new std::string(*other.string_value); break;
            case array: array_value = new std::vector<json>(*other.array_value); break;
            case object: object_value = new std::map<std::string, json>(*other.object_value); break;
        }
    }
    
    void dump_impl(std::ostringstream& oss, int current_indent, int max_indent) const {
        switch (type_) {
            case null: oss << "null"; break;
            case boolean: oss << (bool_value ? "true" : "false"); break;
            case number: oss << number_value; break;
            case string: oss << "\"" << escape_string(*string_value) << "\""; break;
            case array: {
                oss << "[";
                if (max_indent >= 0 && current_indent < max_indent) {
                    oss << "\n" << std::string(current_indent + 1, ' ');
                }
                for (size_t i = 0; i < array_value->size(); ++i) {
                    if (i > 0) {
                        oss << ",";
                        if (max_indent >= 0 && current_indent < max_indent) {
                            oss << "\n" << std::string(current_indent + 1, ' ');
                        } else {
                            oss << " ";
                        }
                    }
                    (*array_value)[i].dump_impl(oss, current_indent + 1, max_indent);
                }
                if (max_indent >= 0 && current_indent < max_indent) {
                    oss << "\n" << std::string(current_indent, ' ');
                }
                oss << "]";
                break;
            }
            case object: {
                oss << "{";
                if (max_indent >= 0 && current_indent < max_indent) {
                    oss << "\n" << std::string(current_indent + 1, ' ');
                }
                bool first = true;
                for (const auto& [key, value] : *object_value) {
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
                break;
            }
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
    
    static json parse_impl(std::istringstream& iss) {
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
            while (iss.get(c) && (std::isdigit(c) || c == '.' || c == 'e' || c == 'E' || c == '+' || c == '-')) {
                num_str += c;
            }
            iss.seekg(-1, std::ios::cur);
            return json(std::stod(num_str));
        }
        
        throw std::runtime_error("Invalid JSON");
    }
};

// Global parse function
json parse(const std::string& str) {
    return json::parse(str);
}

} // namespace nlohmann

