#!/bin/bash

# Script to execute curl command 100 times  
# Tests the echo server with POST requests

echo "Starting 100 curl requests to echo server..."
echo "This test sends requests slowly to test batch timeout behavior"
echo "Target: http://localhost:8080/echo"
echo "----------------------------------------"

success_count=0
failure_count=0

for i in {1..100}; do
    echo -n "Request $i: "
    
    # Execute curl command and capture exit code
    response=$(curl -s -X POST http://localhost:8080/echo \
        -H "Content-Type: application/json" \
        -d '{"hello":"world"}' \
        -w "%{http_code}")
    
    # Check if curl succeeded
    if [ $? -eq 0 ]; then
        # Extract HTTP status code (last 3 characters)
        http_code="${response: -3}"
        response_body="${response%???}"
        
        if [ "$http_code" = "200" ]; then
            echo "SUCCESS (HTTP $http_code)"
            ((success_count++))
        else
            echo "FAILED (HTTP $http_code)"
            ((failure_count++))
        fi
    else
        echo "FAILED (Connection error)"
        ((failure_count++))
    fi
    
    # Small delay between requests (optional)
    sleep 0.1
done

echo "----------------------------------------"
echo "Test completed!"
echo "Successful requests: $success_count"
echo "Failed requests: $failure_count"
echo "Total requests: 100"