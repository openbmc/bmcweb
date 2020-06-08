#pragma once

namespace redfish
{
namespace detail
{

bool validateLength(crow::Response& res, size_t field, std::string_view name,
                    int limit)
{
    if (limit > 0 && field > static_cast<size_t>(limit))
    {
        messages::stringValueTooLong(res, std::string(name), limit);
        return false;
    }
    return true;
}

bool validateLength(crow::Response& res, const std::string& field,
                    std::string_view name, int limit)
{
    return validateLength(res, field.size(), name, limit);
}

bool validateLength(crow::Response& res,
                    const sdbusplus::message::object_path& field,
                    std::string_view name, int limit)
{
    return validateLength(res, field.str.size(), name, limit);
}

template <class T>
bool validateLength(crow::Response& res, const std::vector<T>& fields,
                    std::string_view name, int limit)
{
    return validateLength(res, fields.size(), name, limit) &&
           std::all_of(fields.begin(), fields.end(),
                       [&res, name, limit](const auto& field) {
                           return validateLength(res, field, name, limit);
                       });
}

template <class Tuple, size_t... Seq>
bool validateParamLength(crow::Response& res, Tuple&& tuple,
                         std::index_sequence<Seq...>)
{
    return validateLength(res, std::get<Seq>(tuple)...);
}

template <class Tuple>
bool validateParamLength(crow::Response& res, Tuple&& tuple)
{
    return validateParamLength(
        res, std::forward<Tuple>(tuple),
        std::make_index_sequence<std::tuple_size_v<std::decay_t<Tuple>>>());
}

template <class Limits, size_t... Seq>
bool validateParamsLength(crow::Response& res, Limits&& limits,
                          std::index_sequence<Seq...>)
{
    return (... && validateParamLength(res, std::get<Seq>(limits)));
}
} // namespace detail

template <class Field>
bool validateParamLength(crow::Response& res, Field&& field,
                         std::string_view name, int limit)
{
    return detail::validateLength(res, std::forward<Field>(field), name, limit);
}

template <class Limits>
bool validateParamsLength(crow::Response& res, Limits&& limits)
{
    return detail::validateParamsLength(
        res, std::forward<Limits>(limits),
        std::make_index_sequence<std::tuple_size_v<std::decay_t<Limits>>>());
}
} // namespace redfish
