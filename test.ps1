# Test the Traffic Processor API

Write-Host "=== Testing Traffic Processor API ===" -ForegroundColor Green

$baseUrl = "http://localhost:8080"

# Test 1: GET request
Write-Host "`n1. Testing GET request..." -ForegroundColor Yellow
try {
    $response = Invoke-RestMethod -Uri "$baseUrl" -Method GET
    Write-Host "✓ GET Response received" -ForegroundColor Green
    $response | ConvertTo-Json -Depth 3
} catch {
    Write-Host "✗ GET request failed: $_" -ForegroundColor Red
}

Start-Sleep 1

# Test 2: POST request with JSON
Write-Host "`n2. Testing POST request with JSON..." -ForegroundColor Yellow
$jsonData = @{
    "name" = "John Doe"
    "email" = "john@example.com"
    "timestamp" = Get-Date -Format "yyyy-MM-dd HH:mm:ss"
} | ConvertTo-Json

try {
    $response = Invoke-RestMethod -Uri "$baseUrl" -Method POST -Body $jsonData -ContentType "application/json"
    Write-Host "✓ POST Response received" -ForegroundColor Green
    $response | ConvertTo-Json -Depth 3
} catch {
    Write-Host "✗ POST request failed: $_" -ForegroundColor Red
}

Write-Host "`n3. Check app logs for captured traffic:" -ForegroundColor Cyan
Write-Host "docker logs traffic-processor-simple --tail 10" -ForegroundColor Gray
