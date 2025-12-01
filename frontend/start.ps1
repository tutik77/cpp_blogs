# Скрипт для быстрого запуска фронтенда
# Запуск простого HTTP сервера на Python

Write-Host "🚀 Запуск Blog Platform Frontend..." -ForegroundColor Cyan

# Проверяем, что микросервисы запущены
Write-Host "`n📡 Проверка доступности микросервисов..." -ForegroundColor Yellow

try {
    $authCheck = Invoke-WebRequest -Uri "http://localhost:3000/v1/Auth/healthcheck" -Method GET -TimeoutSec 2 -ErrorAction SilentlyContinue
    Write-Host "✅ Auth Service (3000) - OK" -ForegroundColor Green
} catch {
    Write-Host "❌ Auth Service (3000) - НЕ ДОСТУПЕН" -ForegroundColor Red
    Write-Host "   Убедитесь, что Auth Service запущен на порту 3000" -ForegroundColor Yellow
}

try {
    $appCheck = Invoke-WebRequest -Uri "http://localhost:3001/users/1/followers" -Method GET -TimeoutSec 2 -ErrorAction SilentlyContinue
    Write-Host "✅ App Service (3001) - OK" -ForegroundColor Green
} catch {
    Write-Host "❌ App Service (3001) - НЕ ДОСТУПЕН" -ForegroundColor Red
    Write-Host "   Убедитесь, что App Service запущен на порту 3001" -ForegroundColor Yellow
}

# Путь к frontend
$frontendPath = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $frontendPath

Write-Host "`n🌐 Запуск HTTP сервера на порту 8080..." -ForegroundColor Cyan
Write-Host "📂 Директория: $frontendPath" -ForegroundColor Gray
Write-Host "`n🎉 Фронтенд доступен по адресу: http://localhost:8080" -ForegroundColor Green
Write-Host "📖 Для остановки сервера нажмите Ctrl+C`n" -ForegroundColor Gray

# Открыть браузер
Start-Process "http://localhost:8080"

# Запуск Python HTTP сервера
python -m http.server 8080
