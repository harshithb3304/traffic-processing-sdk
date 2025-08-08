#!/bin/bash

# Traffic Processor SDK - Complete Build and Run Script

echo "=== Traffic Processor SDK (Docker) ==="

# Step 1: Check Docker
echo
echo "1. Checking Docker..."
if ! command -v docker &> /dev/null; then
    echo "ERROR: Docker is not installed or not in PATH. Please install Docker."
    exit 1
fi

if ! docker ps &> /dev/null; then
    echo "ERROR: Docker is not running. Please start Docker."
    exit 1
fi
echo "Docker is running."

# Step 2: Build and start services
echo
echo "2. Building and starting services..."
echo "This will:"
echo "  - Build HTTP server in Docker"
echo "  - Start Kafka"
echo "  - Start HTTP server on port 8080"
docker compose -f docker-compose.simple.yml up --build -d
if [ $? -ne 0 ]; then
    echo "ERROR: Failed to start services"
    exit 1
fi

echo
echo "Services started! Status:"
docker compose -f docker-compose.simple.yml ps

echo
echo "=== READY TO TEST ==="
echo "HTTP Server: http://localhost:8080"
echo
echo "Test commands:"
echo "curl -X POST http://localhost:8080 -H \"Content-Type: application/json\" -d '{\"test\":\"data\"}'"
echo
echo "View app logs:"
echo "docker logs -f traffic-processor-simple"
echo
echo "Stop everything:"
echo "docker compose -f docker-compose.simple.yml down"
