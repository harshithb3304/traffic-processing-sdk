# Real-time log monitoring script

Write-Host "=== Real-Time Traffic Log Monitor ===" -ForegroundColor Green
Write-Host "Watching logs from traffic-processor-simple..." -ForegroundColor Yellow
Write-Host "Send requests to http://localhost:8080 to see live traffic capture!" -ForegroundColor Cyan
Write-Host "Press Ctrl+C to stop monitoring`n" -ForegroundColor Gray

docker logs -f traffic-processor-simple
