#include "struct_proto_conversion.hpp"

namespace google::protobuf
{

void to_json(nlohmann::json& json, const Value& message) noexcept
{
    switch (message.kind_case())
    {
        case Value::kNullValue:
            json = nullptr;
            break;
        case Value::kNumberValue:
            json = message.number_value();
            break;
        case Value::kStringValue:
            json = message.string_value();
            break;
        case Value::kBoolValue:
            json = message.bool_value();
            break;
        case Value::kStructValue:
            json = message.struct_value();
            break;
        case Value::kListValue:
            json = message.list_value();
            break;
        case Value::KIND_NOT_SET:
            json = nlohmann::json::object();
            break;
    }
}

void from_json(Value& message, const nlohmann::json& json) noexcept
{
    if (json.is_null())
    {
        message.set_null_value(NullValue::NULL_VALUE);
    }
    else if (json.is_number())
    {
        auto* dbl_value = json.get_ptr<const double*>();
        if (dbl_value != nullptr)
        {
            message.set_number_value(*dbl_value);
        }
    }
    else if (json.is_string())
    {
        auto* str_value = json.get_ptr<const std::string*>();
        if (str_value != nullptr)
        {
            message.set_string_value(*str_value);
        }
    }
    else if (json.is_boolean())
    {
        auto* bool_value = json.get_ptr<const bool*>();
        if (bool_value != nullptr)
        {
            message.set_bool_value(*bool_value);
        }
    }
    else if (json.is_object())
    {
        from_json(*message.mutable_struct_value(), json);
    }
    else if (json.is_array())
    {
        from_json(*message.mutable_list_value(), json);
    }
}

void to_json(nlohmann::json& json, const Struct& message) noexcept
{
    json = nlohmann::json::object();
    for (auto& [key, value] : message.fields())
    {
        json[key] = value;
    }
}

void from_json(Struct& message, const nlohmann::json& json) noexcept
{
    if (json.is_object())
    {
        auto* fields = message.mutable_fields();
        if (fields != nullptr)
        {
            for (auto& [key, value] : json.items())
            {
                from_json((*fields)[key], value);
            }
        }
    }
}

void to_json(nlohmann::json& json, const ListValue& message) noexcept
{
    json = nlohmann::json::array();
    std::copy(message.values().begin(), message.values().end(), json.begin());
}

void from_json(ListValue& message, const nlohmann::json& json) noexcept
{
    if (json.is_array())
    {
        for (auto& value : json)
        {
            Value* new_value = message.add_values();
            if (new_value != nullptr)
            {
                from_json(*new_value, value);
            }
        }
    }
}

} // namespace google::protobuf