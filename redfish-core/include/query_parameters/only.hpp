#pragma once

#include "error_messages.hpp"
#include "query.hpp"

#include <boost/url/impl/params_view.hpp>
#include <boost/url/params_view.hpp>
#include <boost/url/string.hpp>

namespace redfish::query_parameters
{

class OnlyMemberQuery : public Parameter
{
  public:
    ~OnlyMemberQuery() override = default;

    static std::unique_ptr<Parameter>
        parse(const boost::urls::params_view& urlParams,
              crow::Response& intermediateResponse)
    {
        auto it = urlParams.find(onlyParamString);
        if (it == urlParams.end())
        {
            return nullptr;
        }
        boost::urls::params_view::value_type param = *it;
        if (!param.value.empty())
        {
            messages::queryParameterValueFormatError(
                intermediateResponse, {param.value.begin(), param.value.end()},
                {param.key.begin(), param.key.end()});
            return nullptr;
        }
        return std::unique_ptr<OnlyMemberQuery>(new OnlyMemberQuery());
    }

    ParameterType type() override
    {
        return ParameterType::only;
    }

    void process(const boost::urls::params_view& /*urlParams*/,
                 crow::Response& /*intermediateResponse*/) override
    {
        // TODO: add the actual implementation in the children PRs
    }

  private:
    OnlyMemberQuery() = default;

    static constexpr std::string_view onlyParamString = "only";
};

} // namespace redfish::query_parameters
