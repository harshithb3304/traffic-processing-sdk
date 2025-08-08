#include <iostream>
#include <thread>
#include <string>
#include <sstream>
#include <chrono>
#include <cstring>
#include <iomanip>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>

#include "traffic_processor/sdk.hpp"

using namespace traffic_processor;

class EchoServer {
private:
    int server_fd_;
    bool running_;
    int port_;
    
    // HTTP response templates
    static const std::string HTTP_200_HEADER;
    static const std::string HTTP_404_HEADER;
    
public:
    explicit EchoServer(int port = 8080) : server_fd_(-1), running_(false), port_(port) {}
    
    ~EchoServer() {
        stop();
    }
    
    void start() {
        // Create socket
        server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd_ == -1) {
            throw std::runtime_error("Failed to create socket");
        }
        
        // Set socket options for reuse
        int opt = 1;
        if (setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
            close(server_fd_);
            throw std::runtime_error("Failed to set socket options");
        }
        
        // Bind to address
        struct sockaddr_in address;
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(port_);
        
        if (bind(server_fd_, (struct sockaddr*)&address, sizeof(address)) < 0) {
            close(server_fd_);
            throw std::runtime_error("Failed to bind to port " + std::to_string(port_));
        }
        
        // Listen for connections
        if (listen(server_fd_, 10) < 0) {
            close(server_fd_);
            throw std::runtime_error("Failed to listen on socket");
        }
        
        running_ = true;
        std::cout << "🌐 Echo Server listening on http://0.0.0.0:" << port_ << std::endl;
        std::cout << "📡 Try: curl http://localhost:" << port_ << "/echo" << std::endl;
        std::cout << "📡 Try: curl -X POST http://localhost:" << port_ << "/echo -d '{\"test\":\"data\"}' -H 'Content-Type: application/json'" << std::endl;
        
        // Accept connections loop
        while (running_) {
            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);
            
            int client_socket = accept(server_fd_, (struct sockaddr*)&client_addr, &client_len);
            if (client_socket < 0) {
                if (running_) {
                    std::cerr << "❌ Accept failed" << std::endl;
                }
                continue;
            }
            
            // Handle request in new thread
            std::thread(&EchoServer::handleRequest, this, client_socket, client_addr).detach();
        }
    }
    
    void stop() {
        running_ = false;
        if (server_fd_ >= 0) {
            close(server_fd_);
            server_fd_ = -1;
        }
    }
    
private:
    void handleRequest(int client_socket, const struct sockaddr_in& client_addr) {
        const int BUFFER_SIZE = 4096;
        char buffer[BUFFER_SIZE] = {0};
        
        auto start_time = std::chrono::steady_clock::now();
        
        // Read HTTP request
        ssize_t bytes_read = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
        if (bytes_read <= 0) {
            close(client_socket);
            return;
        }
        
        std::string request(buffer, bytes_read);
        
        // Parse HTTP request line
        std::istringstream request_stream(request);
        std::string method, path, version;
        request_stream >> method >> path >> version;
        
        std::cout << "🔵 HTTP Request: " << method << " " << path << " from " << inet_ntoa(client_addr.sin_addr) << std::endl;
        
        // Extract headers and body
        std::string headers, body;
        size_t header_end = request.find("\r\n\r\n");
        if (header_end != std::string::npos) {
            headers = request.substr(0, header_end);
            body = request.substr(header_end + 4);
        } else {
            headers = request;
        }
        
        // Generate response
        std::string response_body, http_response;
        int status_code;
        
        if (path == "/echo" || path == "/") {
            // Success response
            response_body = createEchoResponse(method, path, body);
            http_response = HTTP_200_HEADER + std::to_string(response_body.length()) + "\r\n\r\n" + response_body;
            status_code = 200;
        } else {
            // 404 Not Found
            response_body = R"({"error":"Not Found","path":")" + path + R"(","message":"Endpoint not found"})";
            http_response = HTTP_404_HEADER + std::to_string(response_body.length()) + "\r\n\r\n" + response_body;
            status_code = 404;
        }
        
        // Send HTTP response
        send(client_socket, http_response.c_str(), http_response.length(), 0);
        
        auto end_time = std::chrono::steady_clock::now();
        
        // Capture traffic for Kafka
        captureTraffic(method, path, headers, body, status_code, response_body, start_time, end_time, client_addr);
        
        close(client_socket);
    }
    
    std::string createEchoResponse(const std::string& method, const std::string& path, const std::string& body) {
        std::ostringstream json;
        json << "{"
             << R"("message":"Traffic Processor Echo Server",)"
             << R"("method":")" << method << R"(",)"
             << R"("path":")" << path << R"(",)"
             << R"("timestamp":")" << getCurrentTimestamp() << R"(",)"
             << R"("received_body":")" << escapeJson(body) << R"(",)"
             << R"("server":"C++ Echo Server")"
             << "}";
        return json.str();
    }
    
    void captureTraffic(const std::string& method, const std::string& path, const std::string& headers,
                       const std::string& body, int status_code, const std::string& response_body,
                       const std::chrono::steady_clock::time_point& start_time,
                       const std::chrono::steady_clock::time_point& end_time,
                       const struct sockaddr_in& client_addr) {
        
        // Build request data
        RequestData req_data;
        req_data.method = method;
        req_data.scheme = "http";
        req_data.host = "localhost:" + std::to_string(port_);
        req_data.path = path;
        req_data.query = "";
        req_data.bodyBase64 = body; // Simplified - not actual base64 encoding
        req_data.ip = inet_ntoa(client_addr.sin_addr);
        req_data.startNs = std::chrono::duration_cast<std::chrono::nanoseconds>(start_time.time_since_epoch()).count();
        
        // Build response data
        ResponseData resp_data;
        resp_data.status = status_code;
        resp_data.bodyBase64 = response_body; // Simplified - not actual base64 encoding
        resp_data.endNs = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time.time_since_epoch()).count();
        
        // Capture with SDK
        TrafficProcessorSdk::instance().capture(req_data, resp_data);
        std::cout << "📊 Traffic captured and queued for Kafka" << std::endl;
    }
    
    std::string getCurrentTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::ostringstream oss;
        oss << std::put_time(std::gmtime(&time_t), "%Y-%m-%d %H:%M:%S UTC");
        return oss.str();
    }
    
    std::string escapeJson(const std::string& input) {
        std::string output;
        for (char c : input) {
            switch (c) {
                case '"': output += "\\\""; break;
                case '\\': output += "\\\\"; break;
                case '\n': output += "\\n"; break;
                case '\r': output += "\\r"; break;
                case '\t': output += "\\t"; break;
                default: output += c; break;
            }
        }
        return output;
    }
};

// Static HTTP response templates
const std::string EchoServer::HTTP_200_HEADER = 
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: application/json\r\n"
    "Connection: close\r\n"
    "Content-Length: ";

const std::string EchoServer::HTTP_404_HEADER = 
    "HTTP/1.1 404 Not Found\r\n"
    "Content-Type: application/json\r\n"
    "Connection: close\r\n"
    "Content-Length: ";

// Global server instance for signal handling
EchoServer* server_instance = nullptr;

void signal_handler(int signal_num) {
    std::cout << "\n🛑 Received signal " << signal_num << ", shutting down gracefully..." << std::endl;
    if (server_instance) {
        server_instance->stop();
    }
}

int main() {
    // Setup signal handling for graceful shutdown
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    try {
        std::cout << "🚀 Starting Traffic Processor SDK Echo Server..." << std::endl;
        
        // Initialize the SDK
        TrafficProcessorSdk::instance().initialize();
        std::cout << "✅ SDK initialized successfully" << std::endl;
        
        // Start the echo server
        EchoServer server(8080);
        server_instance = &server;
        
        server.start();
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Error: " << e.what() << std::endl;
        return 1;
    }
    
    std::cout << "🛑 Echo Server stopped" << std::endl;
    TrafficProcessorSdk::instance().shutdown();
    return 0;
}
