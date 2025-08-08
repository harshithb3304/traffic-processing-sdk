# Traffic Processor SDK - Final Working Solution

Write-Host "=== Traffic Processor SDK (Python/Docker) ===" -ForegroundColor Green

# Check Docker
if (-not (Get-Command docker -ErrorAction SilentlyContinue)) {
    Write-Host "ERROR: Docker not found. Please install Docker Desktop." -ForegroundColor Red
    exit 1
}

Write-Host "`nStarting services..." -ForegroundColor Yellow
docker compose -f docker-compose.simple.yml up -d

Write-Host "`nServices started! Status:" -ForegroundColor Green
docker compose -f docker-compose.simple.yml ps

Write-Host "`n=== READY TO TEST ===" -ForegroundColor Cyan
Write-Host "HTTP Server: http://localhost:8080" -ForegroundColor White
Write-Host "`nTest commands:" -ForegroundColor Yellow
Write-Host 'curl -X POST http://localhost:8080/echo -H "Content-Type: application/json" -d "{\"test\":\"data\"}"' -ForegroundColor Gray
Write-Host "`nView app logs:" -ForegroundColor Yellow
Write-Host "docker logs -f traffic-processor-simple" -ForegroundColor Gray
Write-Host "`nStop everything:" -ForegroundColor Yellow
Write-Host "docker compose -f docker-compose.simple.yml down" -ForegroundColor Gray
