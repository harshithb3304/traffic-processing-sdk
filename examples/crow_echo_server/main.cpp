#include <crow.h>
#include <nlohmann/json.hpp>

#include "traffic_processor/sdk.hpp"

#include <chrono>
#include <iostream>

using namespace traffic_processor;

static std::string maybe_base64(const std::string& body) { 
    return crow::utility::base64encode(body, body.size()); 
}

int main(int argc, char** argv) {
    (void)argc; (void)argv;
    
    std::cout << "Starting Traffic Processor SDK Demo Server..." << std::endl;
    TrafficProcessorSdk::instance().initialize(); // Simple local setup
    std::cout << "SDK initialized successfully" << std::endl;

    crow::SimpleApp app;

    CROW_ROUTE(app, "/echo").methods(crow::HTTPMethod::GET, crow::HTTPMethod::POST)
    ([](const crow::request& req){
        auto start = std::chrono::steady_clock::now().time_since_epoch();
        uint64_t startNs = std::chrono::duration_cast<std::chrono::nanoseconds>(start).count();

        crow::response resp;
        nlohmann::json j;
        j["method"] = crow::method_name(req.method);
        j["body"] = req.body;
        j["url"] = req.url;
        resp.set_header("content-type", "application/json");
        resp.code = 200;
        resp.body = j.dump();

        // Build capture data
        RequestData r;
        r.method = crow::method_name(req.method);
        r.scheme = req.get_header_value("X-Forwarded-Proto");
        if (r.scheme.empty()) r.scheme = "http";
        r.host = req.get_header_value("Host");
        r.path = req.url; // includes path (and may include query); keep simple
        r.query = "";
        nlohmann::json hreq = nlohmann::json::object();
        for (const auto& [k, v] : req.headers) hreq[k] = v;
        r.headers = hreq;
        r.bodyBase64 = maybe_base64(req.body);
        r.ip = req.remote_ip_address;
        r.startNs = startNs;

        ResponseData s;
        s.status = resp.code;
        nlohmann::json hres = nlohmann::json::object();
        hres["content-type"] = "application/json";
        s.headers = hres;
        s.bodyBase64 = crow::utility::base64encode(resp.body, resp.body.size());
        auto end = std::chrono::steady_clock::now().time_since_epoch();
        s.endNs = std::chrono::duration_cast<std::chrono::nanoseconds>(end).count();

        std::cout << "HTTP Request: " << r.method << " " << r.path << " from " << r.ip << std::endl;
        TrafficProcessorSdk::instance().capture(r, s);
        std::cout << "Traffic captured and queued for Kafka" << std::endl;
        return resp;
    });

    std::cout << "Server starting on http://0.0.0.0:8080" << std::endl;
    std::cout << "Try: curl -X POST http://localhost:8080/echo -d '{\"test\":\"data\"}' -H 'Content-Type: application/json'" << std::endl;
    
    app.port(8080).multithreaded().run();
    
    std::cout << "Shutting down..." << std::endl;
    TrafficProcessorSdk::instance().shutdown();
    return 0;
}