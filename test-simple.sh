#!/bin/bash

# Simple request testing script

echo "=== Quick Test Commands ==="

# Test GET
echo
echo "🔵 Testing GET request..."
curl -s http://localhost:8080 | jq . 2>/dev/null || curl -s http://localhost:8080

# Test POST
echo
echo
echo "🔵 Testing POST request..."
curl -s -X POST http://localhost:8080 \
    -H "Content-Type: application/json" \
    -d '{"test":"data","timestamp":"'$(date +"%Y-%m-%d %H:%M:%S")'"}' \
    | jq . 2>/dev/null || curl -s -X POST http://localhost:8080 \
    -H "Content-Type: application/json" \
    -d '{"test":"data","timestamp":"'$(date +"%Y-%m-%d %H:%M:%S")'"}'

echo
echo
echo "📺 View logs: docker logs traffic-processor-simple --tail 20"
