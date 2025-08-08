# Simple request testing script

Write-Host "=== Quick Test Commands ===" -ForegroundColor Green

# Test GET
Write-Host "`n🔵 Testing GET request..." -ForegroundColor Cyan
Invoke-RestMethod -Uri "http://localhost:8080" -Method GET

# Test POST
Write-Host "`n🔵 Testing POST request..." -ForegroundColor Cyan
$jsonBody = '{"test":"data","timestamp":"' + (Get-Date -Format "yyyy-MM-dd HH:mm:ss") + '"}'
Invoke-RestMethod -Uri "http://localhost:8080" -Method POST -Body $jsonBody -ContentType "application/json"

Write-Host "`n📺 View logs: docker logs traffic-processor-simple --tail 20" -ForegroundColor Yellow
