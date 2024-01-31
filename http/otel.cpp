#include "opentelemetry/context/propagation/global_propagator.h"
#include "opentelemetry/context/propagation/text_map_propagator.h"
#include "opentelemetry/exporters/ostream/span_exporter_factory.h"
#include "opentelemetry/sdk/trace/exporter.h"
#include "opentelemetry/sdk/trace/processor.h"
#include "opentelemetry/sdk/trace/simple_processor_factory.h"
#include "opentelemetry/sdk/trace/tracer_context.h"
#include "opentelemetry/sdk/trace/tracer_context_factory.h"
#include "opentelemetry/sdk/trace/tracer_provider_factory.h"
#include "opentelemetry/trace/propagation/http_trace_context.h"
#include "opentelemetry/trace/provider.h"
#include "opentelemetry/trace/semantic_conventions.h"

#include "http_request.hpp"

#include <cstring>
#include <iostream>
#include <vector>

std::shared_ptr<opentelemetry::trace::Tracer>
    get_tracer(std::string tracer_name)
{
    auto provider = opentelemetry::trace::Provider::GetTracerProvider();
    return provider->GetTracer(tracer_name);
}

class HttpTextMapCarrier :
    public opentelemetry::context::propagation::TextMapCarrier
{
    // TODO, need to not break into the actual fields object on Response so we
    // don't leak boost stuff
  public:
    HttpTextMapCarrier(boost::beast::http::fields& headers) : headers_(headers)
    {}

    virtual std::string_view Get(std::string_view key) const noexcept override
    {
        auto it = headers_.find(key);
        if (it != headers_.end())
        {
            return it->value();
        }
        return "";
    }

    virtual void Set(std::string_view key,
                     std::string_view value) noexcept override
    {
        headers_.insert(key, value);
    }

    boost::beast::http::fields& headers_;
};

namespace trace = opentelemetry::trace;
namespace context = opentelemetry::context;

void sendOtel(crow::Request& request)
{
    // start active span
    trace::StartSpanOptions options;
    options.kind = trace::SpanKind::kClient;

    std::string span_name = request.url().path();

    std::string_view methodString = request.methodString();
    std::string_view(methodString.data(), methodString.size());

    std::initializer_list<
        std::pair<std::string_view, opentelemetry::common::AttributeValue>>
        options2 = {
            //{trace::SemanticConventions::kUrlFull, request.url().buffer()},
            //{trace::SemanticConventions::kUrlScheme, request.url().scheme()},
            {trace::SemanticConventions::kHttpRequestMethod,
             request.methodString()}};

    HttpTextMapCarrier carrier(request.fields());

    auto span = get_tracer("bmcweb-http-client")
                    ->StartSpan(span_name, options2, options);
    auto scope = get_tracer("bmcweb-http-client")->WithActiveSpan(span);

    // inject current context into http header
    auto current_ctx = context::RuntimeContext::GetCurrent();
    auto prop =
        context::propagation::GlobalTextMapPropagator::GetGlobalPropagator();
    prop->Inject(carrier, current_ctx);

    /*
        // send http request
        http_client::Result result = http_client->GetNoSsl(url,
       carrier.headers_); if (result)
        {
            // set span attributes
            auto status_code = result.GetResponse().GetStatusCode();
            span->SetAttribute(SemanticConventions::kHttpResponseStatusCode,
                               status_code);
            result.GetResponse().ForEachHeader(
                [&span](std::string_view header_name,
                        std::string_view header_value) {
                span->SetAttribute("http.header." +
       std::string(header_name.data()), header_value); return true;
            });

            if (status_code >= 400)
            {
                span->SetStatus(StatusCode::kError);
            }
        }
        else
        {
            span->SetStatus(
                StatusCode::kError,
                "Response Status :" +
                    std::to_string(static_cast<typename std::underlying_type<
                                       http_client::SessionState>::type>(
                        result.GetSessionState())));
        }
    */
    // end span and export data
    span->End();
}

class OtelTracer
{
    OtelTracer()
    {
        auto exporter = opentelemetry::exporter::trace::
            OStreamSpanExporterFactory::Create();
        auto processor =
            opentelemetry::sdk::trace::SimpleSpanProcessorFactory::Create(
                std::move(exporter));
        std::vector<std::unique_ptr<opentelemetry::sdk::trace::SpanProcessor>>
            processors;
        processors.push_back(std::move(processor));
        // Default is an always-on sampler.
        std::unique_ptr<opentelemetry::sdk::trace::TracerContext> context =
            opentelemetry::sdk::trace::TracerContextFactory::Create(
                std::move(processors));
        std::shared_ptr<opentelemetry::trace::TracerProvider> provider =
            opentelemetry::sdk::trace::TracerProviderFactory::Create(
                std::move(context));
        // Set the global trace provider
        opentelemetry::trace::Provider::SetTracerProvider(provider);

        // set global propagator
        opentelemetry::context::propagation::GlobalTextMapPropagator::
            SetGlobalPropagator(
                std::shared_ptr<
                    opentelemetry::context::propagation::TextMapPropagator>(
                    new opentelemetry::trace::propagation::HttpTraceContext()));
    }

    ~OtelTracer()
    {
        std::shared_ptr<opentelemetry::trace::TracerProvider> none;
        opentelemetry::trace::Provider::SetTracerProvider(none);
    }
};
