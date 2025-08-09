#!/bin/bash

echo "Testing Kafka Batching with 150 requests..."
echo "Batch size is set to 100 messages"
echo "Linger time is 100ms"
echo ""
echo "Expected behavior:"
echo "- Every 2 seconds: You'll see REAL batch statistics from rdkafka"
echo "- Shows actual messages sent vs batches used (e.g., '150 messages in 3 batches')"
echo "- Consistent batching regardless of send speed"
echo ""

# Wait for services to be ready
echo "Waiting for services to start..."
sleep 5

echo "Sending 150 rapid HTTP requests to trigger batching..."

# Send requests in bursts to better demonstrate batching
for i in {1..150}; do
    curl -s -X POST http://localhost:8080/echo \
        -H "Content-Type: application/json" \
        -d "{\"test\":\"batch_test_$i\",\"timestamp\":\"$(date +%s)\",\"request_id\":$i}" \
        > /dev/null &
    
    # Log progress every 25 requests
    if [ $((i % 25)) -eq 0 ]; then
        echo "Sent $i requests..."
    fi
    
    # No delay - send as fast as possible to accumulate in batches
done

# Wait for all background curl processes to complete
wait

echo "All requests sent simultaneously - now waiting 2 seconds for batching..."
sleep 2

echo ""
echo "All 150 requests sent!"
echo "Check the traffic-processor logs to see batching in action:"
echo "   docker logs -f traffic-processor"
echo ""
echo "Expected log patterns:"
echo "   REAL BATCH STATS: 150 messages in 2 batches (avg 75.0 msgs/batch)"
echo "   REAL BATCH STATS: 47 messages in 1 batches (avg 47.0 msgs/batch)"
echo "   (Actual rdkafka batch statistics - not time-based guessing!)"
