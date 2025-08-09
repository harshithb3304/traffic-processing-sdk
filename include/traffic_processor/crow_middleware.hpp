#pragma once

#include <chrono>
#include <string>

#include <crow.h>
#include <nlohmann/json.hpp>

#include "traffic_processor/sdk.hpp"

namespace traffic_processor
{
    namespace crow_integration
    {

        // Reusable Crow middleware that captures every request/response
        // and forwards it to the Traffic Processor SDK.
        struct TrafficMiddleware
        {
            struct context
            {
                std::chrono::steady_clock::duration start_time;
            };

            static std::string maybe_base64(const std::string &body)
            {
                return crow::utility::base64encode(body, body.size());
            }

            void before_handle(crow::request & /*req*/, crow::response & /*res*/, context &ctx)
            {
                ctx.start_time = std::chrono::steady_clock::now().time_since_epoch();
            }

            void after_handle(crow::request &req, crow::response &res, context &ctx)
            {
                auto start = ctx.start_time;
                uint64_t startNs = std::chrono::duration_cast<std::chrono::nanoseconds>(start).count();

                RequestData r;
                r.method = crow::method_name(req.method);
                r.scheme = req.get_header_value("X-Forwarded-Proto");
                if (r.scheme.empty())
                    r.scheme = "http";
                r.host = req.get_header_value("Host");
                r.path = req.url;
                r.query = "";
                nlohmann::json hreq = nlohmann::json::object();
                for (const auto &[k, v] : req.headers)
                    hreq[k] = v;
                r.headers = hreq;
                r.bodyText = req.body;
                r.bodyBase64 = maybe_base64(req.body);
                r.ip = req.remote_ip_address;
                r.startNs = startNs;

                ResponseData s;
                s.status = res.code;
                nlohmann::json hres = nlohmann::json::object();
                for (const auto &[k, v] : res.headers)
                    hres[k] = v;
                s.headers = hres;
                s.bodyText = res.body;
                s.bodyBase64 = crow::utility::base64encode(res.body, res.body.size());
                auto end = std::chrono::steady_clock::now().time_since_epoch();
                s.endNs = std::chrono::duration_cast<std::chrono::nanoseconds>(end).count();

                TrafficProcessorSdk::instance().capture(r, s);
            }
        };

        // Convenience alias to create an app with the middleware baked in
        using TrafficApp = crow::App<TrafficMiddleware>;

    } // namespace crow_integration
} // namespace traffic_processor
