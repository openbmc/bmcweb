// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "filter_expr_executor.hpp"

#include "filter_expr_parser_ast.hpp"
#include "human_sort.hpp"
#include "logging.hpp"
#include "utils/time_utils.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <variant>

namespace redfish
{

namespace
{

// A value that has been parsed as a time string per Edm.DateTimeOffset
struct DateTimeString
{
    time_utils::usSinceEpoch value = time_utils::usSinceEpoch::zero();

    // The following is created by dumping all key names of type
    // Edm.DateTimeOffset.  While imperfect that it's a hardcoded list, these
    // keys don't change that often
    static constexpr auto timeKeys = std::to_array<std::string_view>(
        {"AccountExpiration",
         "CalibrationTime",
         "CoefficientUpdateTime",
         "Created",
         "CreatedDate",
         "CreatedTime",
         "CreateTime",
         "DateTime",
         "EndDateTime",
         "EndTime",
         "EventTimestamp",
         "ExpirationDate",
         "FirstOverflowTimestamp",
         "InitialStartTime",
         "InstallDate",
         "LastOverflowTimestamp",
         "LastResetTime",
         "LastStateTime",
         "LastUpdated",
         "LifetimeStartDateTime",
         "LowestReadingTime",
         "MaintenanceWindowStartTime",
         "Modified",
         "PasswordExpiration",
         "PeakReadingTime",
         "PresentedPublicHostKeyTimestamp",
         "ProductionDate",
         "ReadingTime",
         "ReleaseDate",
         "ReservationTime",
         "SensorResetTime",
         "ServicedDate",
         "SetPointUpdateTime",
         "StartDateTime",
         "StartTime",
         "Time",
         "Timestamp",
         "ValidNotAfter",
         "ValidNotBefore"});

    explicit DateTimeString(std::string_view strvalue)
    {
        std::optional<time_utils::usSinceEpoch> out =
            time_utils::dateStringToEpoch(strvalue);
        if (!out)
        {
            BMCWEB_LOG_ERROR(
                "Internal datetime value didn't parse as datetime?");
        }
        else
        {
            value = *out;
        }
    }

    static bool isDateTimeKey(std::string_view key)
    {
        auto out = std::equal_range(timeKeys.begin(), timeKeys.end(), key);
        return out.first != out.second;
    }
};

// Class that can convert an arbitrary AST type into a structured value
// Pulling from the json pointer when required
struct ValueVisitor
{
    using result_type = std::variant<std::monostate, double, int64_t,
                                     std::string, DateTimeString>;
    const nlohmann::json& body;
    result_type operator()(double n);
    result_type operator()(int64_t x);
    result_type operator()(const filter_ast::UnquotedString& x);
    result_type operator()(const filter_ast::QuotedString& x);
};

ValueVisitor::result_type ValueVisitor::operator()(double n)
{
    return {n};
}

ValueVisitor::result_type ValueVisitor::operator()(int64_t x)
{
    return {x};
}

ValueVisitor::result_type
    ValueVisitor::operator()(const filter_ast::QuotedString& x)
{
    return {x};
}

ValueVisitor::result_type
    ValueVisitor::operator()(const filter_ast::UnquotedString& x)
{
    // Future, handle paths with / in them
    nlohmann::json::const_iterator entry = body.find(x);
    if (entry == body.end())
    {
        BMCWEB_LOG_ERROR("Key {} doesn't exist in output, cannot filter",
                         static_cast<std::string>(x));
        BMCWEB_LOG_DEBUG("Output {}", body.dump());
        return {};
    }
    const double* dValue = entry->get_ptr<const double*>();
    if (dValue != nullptr)
    {
        return {*dValue};
    }
    const int64_t* iValue = entry->get_ptr<const int64_t*>();
    if (iValue != nullptr)
    {
        return {*iValue};
    }
    const std::string* strValue = entry->get_ptr<const std::string*>();
    if (strValue != nullptr)
    {
        if (DateTimeString::isDateTimeKey(x))
        {
            return DateTimeString(*strValue);
        }
        return {*strValue};
    }

    BMCWEB_LOG_ERROR(
        "Type for key {} was {} which does not have a comparison operator",
        static_cast<std::string>(x), static_cast<int>(entry->type()));
    return {};
}

struct ApplyFilter
{
    const nlohmann::json& body;
    const filter_ast::LogicalAnd& filter;
    using result_type = bool;
    bool operator()(const filter_ast::LogicalNot& x);
    bool operator()(const filter_ast::LogicalOr& x);
    bool operator()(const filter_ast::LogicalAnd& x);
    bool operator()(const filter_ast::Comparison& x);
    bool operator()(const filter_ast::BooleanOp& x);

  public:
    bool matches();
};

bool ApplyFilter::operator()(const filter_ast::LogicalNot& x)
{
    bool subValue = (*this)(x.operand);
    if (x.isLogicalNot)
    {
        return !subValue;
    }
    return subValue;
}

// Helper function to reduce the number of permutations of a single comparison
// For all possible types.
bool doDoubleComparison(double left, filter_ast::ComparisonOpEnum comparator,
                        double right)
{
    if (!std::isfinite(left) || !std::isfinite(right))
    {
        BMCWEB_LOG_ERROR("Refusing to do comparision of non finite numbers");
        return false;
    }
    switch (comparator)
    {
        case filter_ast::ComparisonOpEnum::Equals:
            // Note, floating point comparisons are hard.  Compare equality
            // based on epsilon
            return std::fabs(left - right) <=
                   std::numeric_limits<double>::epsilon();
        case filter_ast::ComparisonOpEnum::NotEquals:
            return std::fabs(left - right) >
                   std::numeric_limits<double>::epsilon();
        case filter_ast::ComparisonOpEnum::GreaterThan:
            return left > right;
        case filter_ast::ComparisonOpEnum::GreaterThanOrEqual:
            return left >= right;
        case filter_ast::ComparisonOpEnum::LessThan:
            return left < right;
        case filter_ast::ComparisonOpEnum::LessThanOrEqual:
            return left <= right;
        default:
            BMCWEB_LOG_ERROR("Got x.token that should never happen {}",
                             static_cast<int>(comparator));
            return true;
    }
}

bool doIntComparison(int64_t left, filter_ast::ComparisonOpEnum comparator,
                     int64_t right)
{
    switch (comparator)
    {
        case filter_ast::ComparisonOpEnum::Equals:
            return left == right;
        case filter_ast::ComparisonOpEnum::NotEquals:
            return left != right;
        case filter_ast::ComparisonOpEnum::GreaterThan:
            return left > right;
        case filter_ast::ComparisonOpEnum::GreaterThanOrEqual:
            return left >= right;
        case filter_ast::ComparisonOpEnum::LessThan:
            return left < right;
        case filter_ast::ComparisonOpEnum::LessThanOrEqual:
            return left <= right;
        default:
            BMCWEB_LOG_ERROR("Got comparator that should never happen {}",
                             static_cast<int>(comparator));
            return true;
    }
}

bool doStringComparison(std::string_view left,
                        filter_ast::ComparisonOpEnum comparator,
                        std::string_view right)
{
    switch (comparator)
    {
        case filter_ast::ComparisonOpEnum::Equals:
            return left == right;
        case filter_ast::ComparisonOpEnum::NotEquals:
            return left != right;
        case filter_ast::ComparisonOpEnum::GreaterThan:
            return alphanumComp(left, right) > 0;
        case filter_ast::ComparisonOpEnum::GreaterThanOrEqual:
            return alphanumComp(left, right) >= 0;
        case filter_ast::ComparisonOpEnum::LessThan:
            return alphanumComp(left, right) < 0;
        case filter_ast::ComparisonOpEnum::LessThanOrEqual:
            return alphanumComp(left, right) <= 0;
        default:
            BMCWEB_LOG_ERROR(
                "Got comparator that should never happen.  Attempt to do numeric comparison on string {}",
                static_cast<int>(comparator));
            return true;
    }
}

bool ApplyFilter::operator()(const filter_ast::Comparison& x)
{
    ValueVisitor numeric(body);
    std::variant<std::monostate, double, int64_t, std::string, DateTimeString>
        left = boost::apply_visitor(numeric, x.left);
    std::variant<std::monostate, double, int64_t, std::string, DateTimeString>
        right = boost::apply_visitor(numeric, x.right);

    // Numeric comparisons
    const double* lDoubleValue = std::get_if<double>(&left);
    const double* rDoubleValue = std::get_if<double>(&right);
    const int64_t* lIntValue = std::get_if<int64_t>(&left);
    const int64_t* rIntValue = std::get_if<int64_t>(&right);

    if (lDoubleValue != nullptr)
    {
        if (rDoubleValue != nullptr)
        {
            // Both sides are doubles, do the comparison as doubles
            return doDoubleComparison(*lDoubleValue, x.token, *rDoubleValue);
        }
        if (rIntValue != nullptr)
        {
            // If right arg is int, promote right arg to double
            return doDoubleComparison(*lDoubleValue, x.token,
                                      static_cast<double>(*rIntValue));
        }
    }
    if (lIntValue != nullptr)
    {
        if (rIntValue != nullptr)
        {
            // Both sides are ints, do the comparison as ints
            return doIntComparison(*lIntValue, x.token, *rIntValue);
        }

        if (rDoubleValue != nullptr)
        {
            // Left arg is int, promote left arg to double
            return doDoubleComparison(static_cast<double>(*lIntValue), x.token,
                                      *rDoubleValue);
        }
    }

    // String comparisons
    const std::string* lStrValue = std::get_if<std::string>(&left);
    const std::string* rStrValue = std::get_if<std::string>(&right);

    const DateTimeString* lDateValue = std::get_if<DateTimeString>(&left);
    const DateTimeString* rDateValue = std::get_if<DateTimeString>(&right);

    // If we're trying to compare a date string to a string, construct a
    // datestring from the string
    if (lDateValue != nullptr && rStrValue != nullptr)
    {
        rDateValue = &right.emplace<DateTimeString>(std::string(*rStrValue));
    }
    if (lStrValue != nullptr && rDateValue != nullptr)
    {
        lDateValue = &left.emplace<DateTimeString>(std::string(*lStrValue));
    }

    if (lDateValue != nullptr && rDateValue != nullptr)
    {
        return doIntComparison(lDateValue->value.count(), x.token,
                               rDateValue->value.count());
    }

    if (lStrValue != nullptr && rStrValue != nullptr)
    {
        return doStringComparison(*lStrValue, x.token, *rStrValue);
    }

    BMCWEB_LOG_ERROR(
        "Fell through.  Should never happen.  Attempt to compare type {} to type {}",
        static_cast<int>(left.index()), static_cast<int>(right.index()));
    return true;
}

bool ApplyFilter::operator()(const filter_ast::BooleanOp& x)
{
    return boost::apply_visitor(*this, x);
}

bool ApplyFilter::operator()(const filter_ast::LogicalOr& x)
{
    bool value = (*this)(x.first);
    for (const filter_ast::LogicalNot& bOp : x.rest)
    {
        value = value || (*this)(bOp);
    }
    return value;
}

bool ApplyFilter::operator()(const filter_ast::LogicalAnd& x)
{
    bool value = (*this)(x.first);
    for (const filter_ast::LogicalOr& bOp : x.rest)
    {
        value = value && (*this)(bOp);
    }
    return value;
}

bool ApplyFilter::matches()
{
    return (*this)(filter);
}

} // namespace

bool memberMatches(const nlohmann::json& member,
                   const filter_ast::LogicalAnd& filterParam)
{
    ApplyFilter filterApplier(member, filterParam);
    return filterApplier.matches();
}

// Applies a filter expression to a member array
bool applyFilterToCollection(nlohmann::json& body,
                             const filter_ast::LogicalAnd& filterParam)
{
    using nlohmann::json;

    json::object_t* obj = body.get_ptr<json::object_t*>();
    if (obj == nullptr)
    {
        BMCWEB_LOG_ERROR("Requested filter wasn't an object????");
        return false;
    }
    json::object_t::iterator members = obj->find("Members");
    if (members == obj->end())
    {
        BMCWEB_LOG_ERROR("Collection didn't have members?");
        return false;
    }
    json::array_t* memberArr = members->second.get_ptr<json::array_t*>();
    if (memberArr == nullptr)
    {
        BMCWEB_LOG_ERROR("Members wasn't an object????");
        return false;
    }

    json::array_t::iterator it = memberArr->begin();
    size_t index = 0;
    while (it != memberArr->end())
    {
        if (!memberMatches(*it, filterParam))
        {
            BMCWEB_LOG_DEBUG("Removing item at index {}", index);
            it = memberArr->erase(it);
            index++;
            continue;
        }
        it++;
        index++;
    }

    return true;
}
} // namespace redfish
