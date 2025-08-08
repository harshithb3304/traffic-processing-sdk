#!/bin/bash

# Watch Logs Script for Traffic Processor SDK

echo "=== Real-Time Traffic Processor Logs ==="
echo "Watching logs for 'traffic-processor-simple' container..."
echo "Press Ctrl+C to stop."
echo "========================================"

docker logs -f traffic-processor-simple
