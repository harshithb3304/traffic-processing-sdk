#!/bin/bash

# Comprehensive HTTP test script
# Tests all HTTP methods including supported and unsupported ones

echo "Starting comprehensive HTTP method tests..."
echo "Target: http://localhost:8080/echo"
echo "Testing both supported (GET, POST) and unsupported methods"
echo "========================================================"

success_count=0
failure_count=0
total_count=0

# Function to test HTTP method
test_method() {
    local method=$1
    local data=$2
    local description=$3
    
    total_count=$((total_count + 1))
    echo -n "Test $total_count - $method ($description): "
    
    if [ -n "$data" ]; then
        response=$(curl -s -X $method http://localhost:8080/echo \
            -H "Content-Type: application/json" \
            -d "$data" \
            -w "%{http_code}" \
            --max-time 5)
    else
        response=$(curl -s -X $method http://localhost:8080/echo \
            -H "Content-Type: application/json" \
            -w "%{http_code}" \
            --max-time 5)
    fi
    
    curl_exit_code=$?
    
    if [ $curl_exit_code -eq 0 ]; then
        http_code="${response: -3}"
        response_body="${response%???}"
        
        if [ "$http_code" = "200" ]; then
            echo "SUCCESS (HTTP $http_code)"
            success_count=$((success_count + 1))
        elif [ "$http_code" = "404" ] || [ "$http_code" = "405" ]; then
            echo "EXPECTED_ERROR (HTTP $http_code - Method not supported)"
            success_count=$((success_count + 1))
        else
            echo "UNEXPECTED (HTTP $http_code)"
            failure_count=$((failure_count + 1))
        fi
    else
        echo "FAILED (Connection error or timeout)"
        failure_count=$((failure_count + 1))
    fi
    
    # Small delay between requests
    sleep 0.1
}

echo "Testing SUPPORTED methods (should work):"
echo "----------------------------------------"

# Test GET method (supported)
for i in {1..10}; do
    test_method "GET" "" "supported method #$i"
done

# Test POST method (supported)
for i in {1..10}; do
    test_method "POST" '{"test":"post_data","iteration":'$i'}' "supported method #$i"
done

echo ""
echo "Testing UNSUPPORTED methods (should return 404/405):"
echo "---------------------------------------------------"

# Test PUT method (likely unsupported)
for i in {1..5}; do
    test_method "PUT" '{"test":"put_data","iteration":'$i'}' "unsupported method #$i"
done

# Test DELETE method (likely unsupported)
for i in {1..5}; do
    test_method "DELETE" "" "unsupported method #$i"
done

# Test PATCH method (likely unsupported)
for i in {1..5}; do
    test_method "PATCH" '{"test":"patch_data","iteration":'$i'}' "unsupported method #$i"
done

# Test HEAD method (likely unsupported)
for i in {1..3}; do
    test_method "HEAD" "" "unsupported method #$i"
done

# Test OPTIONS method (likely unsupported)
for i in {1..3}; do
    test_method "OPTIONS" "" "unsupported method #$i"
done

echo ""
echo "Testing EDGE CASES:"
echo "------------------"

# Test with large payload
large_payload='{"large_data":"'$(printf 'A%.0s' {1..1000})'","test":"large_payload"}'
test_method "POST" "$large_payload" "large payload"

# Test with special characters
special_payload='{"special":"àáâãäåæçèéêëìíîïñòóôõöøùúûüý","unicode":"🚀🎉","test":"special_chars"}'
test_method "POST" "$special_payload" "special characters"

# Test with empty payload
test_method "POST" '{}' "empty payload"

# Test with malformed JSON
test_method "POST" '{"malformed":json}' "malformed JSON"

# Test non-existent endpoint
echo -n "Test $((total_count + 1)) - POST (non-existent endpoint): "
response=$(curl -s -X POST http://localhost:8080/nonexistent \
    -H "Content-Type: application/json" \
    -d '{"test":"nonexistent"}' \
    -w "%{http_code}" \
    --max-time 5)
    
if [ $? -eq 0 ]; then
    http_code="${response: -3}"
    if [ "$http_code" = "404" ]; then
        echo "SUCCESS (HTTP $http_code - Expected 404)"
        success_count=$((success_count + 1))
    else
        echo "UNEXPECTED (HTTP $http_code)"
        failure_count=$((failure_count + 1))
    fi
else
    echo "FAILED (Connection error)"
    failure_count=$((failure_count + 1))
fi
total_count=$((total_count + 1))

echo ""
echo "========================================================"
echo "Comprehensive test completed!"
echo "Total tests: $total_count"
echo "Successful/Expected: $success_count"
echo "Failed/Unexpected: $failure_count"
echo ""
echo "Check server logs for Kafka batching information:"
echo "  docker logs -f traffic-processor"
echo ""
echo "Expected Kafka logs:"
echo "  KAFKA QUEUE: Message 25 queued, queue depth: X, batch config: 100 msgs/10000ms"
echo "  KAFKA BATCH DELIVERED: X messages in Yms (Total delivered: Z)"
