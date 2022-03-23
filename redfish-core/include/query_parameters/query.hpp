#pragma once

#include "async_resp.hpp"
#include "only.hpp"

#include <boost/url/impl/params_view.hpp>
#include <boost/url/params_view.hpp>

#include <memory>

namespace redfish::query_parameters
{

enum class ParameterType
{
    only,
};

// The interface for all query parameters.
// Derived class shall implement the |parse| factory function.
class Parameter
{
  public:
    virtual ~Parameter() = default;

    // Parses the |urlParams|, sets |intermediateResponse| to errros on failure
    // otherwise returns a parsed parameter.
    template <typename Derived>
    static std::unique_ptr<Parameter>
        parse(const boost::urls::params_view& urlParams,
              crow::Response& intermediateResponse)
    {
        return Derived::parse(urlParams, intermediateResponse);
    }

    // Hijacks the |intermediateResponse| and processes the query parameter.
    virtual void process(const boost::urls::params_view& urlParams,
                         crow::Response& intermediateResponse) = 0;
    // Returns the type of the implemented query parameter.
    virtual ParameterType type() = 0;
};

class ParametersHandler
{
  public:
    // TODO: this will parse all the query parameters and store them into a
    // container. It calls individual parameter's |parse| function.
    void parseParameters()
    {}
    // TODO: this will reorder all parameters according to the spec. It raises
    // errors if the conbination of parameters are not supported.
    // It calls individual parameter's |type| function.
    void reorderParameters()
    {}
    // TODO: this will process the reordered parameters by calling individual
    // parameter's |process| function.
    void processParameters()
    {}
};

} // namespace redfish::query_parameters
