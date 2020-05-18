#pragma once

namespace redfish
{
namespace detail
{
template <class Limits, size_t... Seq>
bool validateParamsLength(crow::Response& res, Limits&& limits,
                          std::index_sequence<Seq...>)
{
    return (... && std::get<Seq>(limits).validate(res));
}
} // namespace detail

template <class T>
struct ItemSizeValidator
{
    ItemSizeValidator(const T&& item, std::string_view name, size_t limit) :
        item(std::forward<const T>(item)), name(name), limit(limit)
    {}

    bool validate(crow::Response& res) const
    {
        return ItemSizeValidator<T>::validateItem(res, item, name, limit);
    }

  private:
    static bool validateItem(crow::Response& res, size_t item,
                             std::string_view name, size_t limit)
    {
        if (item > static_cast<size_t>(limit))
        {
            messages::stringValueTooLong(res, std::string(name),
                                         static_cast<int>(limit));
            return false;
        }
        return true;
    }

    static bool validateItem(crow::Response& res, std::string_view item,
                             std::string_view name, size_t limit)
    {
        return validateItem(res, item.size(), name, limit);
    }

    static bool validateItem(crow::Response& res, const std::string& item,
                             std::string_view name, size_t limit)
    {
        return validateItem(res, item.size(), name, limit);
    }

    static bool validateItem(crow::Response& res,
                             const sdbusplus::message::object_path& item,
                             std::string_view name, size_t limit)
    {
        return validateItem(res, item.str.size(), name, limit);
    }

    T item;
    std::string_view name;
    size_t limit;
};

template <class T>
ItemSizeValidator(const T&, std::string_view, size_t)
    -> ItemSizeValidator<const T&>;

ItemSizeValidator(const char*, std::string_view, size_t)
    ->ItemSizeValidator<std::string_view>;

template <class ContainerT>
struct ArrayItemsValidator
{
    ArrayItemsValidator(const ContainerT& item, std::string_view name,
                        size_t limit) :
        item(item),
        name(name), limit(limit)
    {}

    bool validate(crow::Response& res) const
    {
        return std::all_of(
            item.begin(), item.end(), [&res, this](const auto& item) {
                return ItemSizeValidator(item, name, limit).validate(res);
            });
    }

  private:
    const ContainerT& item;
    std::string_view name;
    size_t limit;
};

template <class T>
bool validateParamLength(crow::Response& res, T&& item, std::string_view name,
                         size_t limit)
{
    return ItemSizeValidator(std::forward<T>(item), name, limit).validate(res);
}

template <class Limits>
bool validateParamsLength(crow::Response& res, Limits&& limits)
{
    return detail::validateParamsLength(
        res, std::forward<Limits>(limits),
        std::make_index_sequence<std::tuple_size_v<std::decay_t<Limits>>>());
}

} // namespace redfish
