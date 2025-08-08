#!/bin/bash

# Test Script for Traffic Processor SDK API

echo "=== Testing Traffic Processor API ==="

# Test 1: GET request
echo
echo "1. Testing GET request..."
response=$(curl -s http://localhost:8080)
if [ $? -eq 0 ]; then
    echo "✔ GET Response received"
    echo "$response" | jq . 2>/dev/null || echo "$response"
else
    echo "✖ GET request failed"
fi

sleep 1

# Test 2: POST request with JSON
echo
echo "2. Testing POST request with JSON..."
response=$(curl -s -X POST http://localhost:8080 \
    -H "Content-Type: application/json" \
    -d '{"name":"John Doe","email":"john@example.com","timestamp":"'$(date +"%Y-%m-%d %H:%M:%S")'"}')
if [ $? -eq 0 ]; then
    echo "✔ POST Response received"
    echo "$response" | jq . 2>/dev/null || echo "$response"
else
    echo "✖ POST request failed"
fi

sleep 1

# Test 3: Multiple rapid requests
echo
echo "3. Testing multiple rapid requests..."
for i in {1..3}; do
    response=$(curl -s -X POST http://localhost:8080 \
        -H "Content-Type: application/json" \
        -d '{"request_id":'$i',"message":"Rapid test '$i'","batch_test":true}')
    if [ $? -eq 0 ]; then
        echo "✔ Request $i sent"
    else
        echo "✖ Request $i failed"
    fi
    sleep 0.5
done

echo
echo "3. Check app logs for captured traffic:"
echo "docker logs traffic-processor-simple --tail 10"
