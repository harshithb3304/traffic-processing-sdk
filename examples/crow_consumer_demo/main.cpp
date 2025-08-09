#include <crow.h>
#include <nlohmann/json.hpp>
#include "traffic_processor/sdk.hpp"

using namespace traffic_processor;

int main()
{
    SdkConfig cfg;
    if (const char *url = std::getenv("KAFKA_URL"))
        cfg.kafka.bootstrapServers = url;
    else
        cfg.kafka.bootstrapServers = "kafka:19092";
    if (const char *topic = std::getenv("KAFKA_TOPIC"))
        cfg.kafka.topic = topic;
    else
        cfg.kafka.topic = "http.traffic";
    cfg.kafka.lingerMs = 750;
    cfg.kafka.batchNumMessages = 1;

    TrafficProcessorSdk::instance().initialize(cfg);

    struct TrafficMiddleware
    {
        struct context
        {
            std::chrono::steady_clock::time_point start;
        };
        void before_handle(crow::request &, crow::response &, context &ctx) { ctx.start = std::chrono::steady_clock::now(); }
        void after_handle(crow::request &req, crow::response &res, context &ctx)
        {
            RequestData r;
            r.method = crow::method_name(req.method);
            r.scheme = "http";
            r.host = req.get_header_value("Host");
            r.path = req.url;
            r.query = "";
            nlohmann::json hreq = nlohmann::json::object();
            for (const auto &[k, v] : req.headers)
                hreq[k] = v;
            r.headers = hreq;
            r.bodyText = req.body;
            r.bodyBase64 = crow::utility::base64encode(req.body, req.body.size());
            r.ip = req.remote_ip_address;
            r.startNs = std::chrono::duration_cast<std::chrono::nanoseconds>(ctx.start.time_since_epoch()).count();
            ResponseData s;
            s.status = res.code;
            nlohmann::json hres = nlohmann::json::object();
            for (const auto &[k, v] : res.headers)
                hres[k] = v;
            s.headers = hres;
            s.bodyText = res.body;
            s.bodyBase64 = crow::utility::base64encode(res.body, res.body.size());
            s.endNs = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
            TrafficProcessorSdk::instance().capture(r, s);
        }
    };

    crow::App<TrafficMiddleware> app;
    CROW_ROUTE(app, "/echo").methods(crow::HTTPMethod::GET, crow::HTTPMethod::POST)([](const crow::request &req)
                                                                                    { crow::response resp; nlohmann::json j; j["method"]=crow::method_name(req.method); j["url"]=req.url; j["body"]=req.body; resp.set_header("content-type","application/json"); resp.code=200; resp.body=j.dump(); return resp; });
    app.port(8080).multithreaded().run();
    TrafficProcessorSdk::instance().shutdown();
    return 0;
}
