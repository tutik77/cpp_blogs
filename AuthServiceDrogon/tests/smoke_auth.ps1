param(
    [string]$BaseUrl = "http://localhost:3000"
)

$ErrorActionPreference = "Stop"

function Write-Title($text) {
    Write-Host "== $text ==" -ForegroundColor Cyan
}

function Assert-True($condition, $message) {
    if (-not $condition) {
        throw "ASSERT FAILED: $message"
    }
}

function Invoke-JsonPost($url, $bodyTable) {
    $json = $bodyTable | ConvertTo-Json -Depth 5
    return Invoke-RestMethod -Uri $url -Method Post -Body $json -ContentType "application/json"
}

Write-Title "Auth service smoke tests"
Write-Host "Base URL: $BaseUrl`n"

try {
    # 1. Healthcheck
    Write-Title "Healthcheck"
    $health = Invoke-RestMethod -Uri "$BaseUrl/v1/Auth/healthcheck" -Method Get
    Write-Host "Response:" ($health | ConvertTo-Json -Depth 5)
    Assert-True ($health.status -eq "ok") "healthcheck.status should be 'ok'"

    # 2. Registration with random login to avoid collisions
    Write-Title "Registration"
    $login = "testuser_" + [Guid]::NewGuid().ToString("N").Substring(0, 8)
    $password = "P@ssw0rd123!"

    $regBody = @{
        name     = "Test User"
        login    = $login
        password = $password
    }

    $regResponse = Invoke-JsonPost "$BaseUrl/v1/Auth/reg" $regBody
    Write-Host "Registered login: $login"
    Write-Host "Response:" ($regResponse | ConvertTo-Json -Depth 5)
    Assert-True ($regResponse.access_token) "reg response should contain access_token"

    # 3. Login with the same credentials
    Write-Title "Login"
    $loginBody = @{
        login    = $login
        password = $password
    }

    $loginResponse = Invoke-JsonPost "$BaseUrl/v1/Auth/login" $loginBody
    Write-Host "Response:" ($loginResponse | ConvertTo-Json -Depth 5)
    Assert-True ($loginResponse.access_token) "login response should contain access_token"

    Write-Host "`nALL TESTS PASSED" -ForegroundColor Green
    exit 0
}
catch {
    Write-Host "`nTEST FAILED:" $_ -ForegroundColor Red
    exit 1
}


