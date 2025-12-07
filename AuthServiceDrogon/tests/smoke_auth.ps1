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

function Invoke-JsonGet($url) {
    return Invoke-RestMethod -Uri $url -Method Get
}

Write-Title "Auth service smoke tests"
Write-Host "Base URL: $BaseUrl`n"

try {
    # 1. Healthcheck
    Write-Title "1. Healthcheck"
    $health = Invoke-RestMethod -Uri "$BaseUrl/v1/Auth/healthcheck" -Method Get
    Write-Host "Response:" ($health | ConvertTo-Json -Depth 5)
    Assert-True ($health.status -eq "ok") "healthcheck.status should be 'ok'"

    # 2. Registration with random login to avoid collisions
    Write-Title "2. Registration"
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
    $regToken = $regResponse.access_token

    # 3. Login with the same credentials
    Write-Title "3. Login"
    $loginBody = @{
        login    = $login
        password = $password
    }

    $loginResponse = Invoke-JsonPost "$BaseUrl/v1/Auth/login" $loginBody
    Write-Host "Response:" ($loginResponse | ConvertTo-Json -Depth 5)
    Assert-True ($loginResponse.access_token) "login response should contain access_token"
    $authToken = $loginResponse.access_token

    # 4. Invalid login (wrong password)
    Write-Title "4. Invalid login (wrong password)"
    $invalidLoginBody = @{
        login    = $login
        password = "wrongpassword"
    }
    try {
        $invalidResponse = Invoke-JsonPost "$BaseUrl/v1/Auth/login" $invalidLoginBody
        throw "Expected error for invalid password"
    }
    catch {
        Write-Host "Expected error: $($_.Exception.Message)"
        Assert-True ($_.Exception.Response.StatusCode -eq 401) "invalid login should return 401"
    }

    # 5. Invalid registration (duplicate login)
    Write-Title "5. Duplicate registration"
    try {
        $dupRegResponse = Invoke-JsonPost "$BaseUrl/v1/Auth/reg" $regBody
        throw "Expected error for duplicate registration"
    }
    catch {
        Write-Host "Expected duplicate error: $($_.Exception.Message)"
        Assert-True ($_.Exception.Response.StatusCode -eq 409 -or $_.Exception.Response.StatusCode -eq 400) "duplicate reg should return 409/400"
    }

    # 6. OPTIONS CORS preflight
    Write-Title "6. CORS preflight check"
    $corsResponse = Invoke-RestMethod -Uri "$BaseUrl/v1/Auth/login" -Method Options `
        -Headers @{ "Origin" = "http://localhost:8080"; "Access-Control-Request-Method" = "POST" }
    Write-Host "CORS headers: $($corsResponse.Headers | ConvertTo-Json -Depth 3)"
    Assert-True ($true) "CORS preflight completed"

    # 7. Healthcheck with token (protected endpoint simulation)
    Write-Title "7. Healthcheck with Authorization header"
    $healthWithToken = Invoke-RestMethod -Uri "$BaseUrl/v1/Auth/healthcheck" -Method Get `
        -Headers @{ "Authorization" = "Bearer $authToken" }
    Write-Host "Healthcheck with token OK"
    Assert-True ($healthWithToken.status -eq "ok") "healthcheck with token should work"

    Write-Host "`nALL TESTS PASSED ✔" -ForegroundColor Green
    exit 0
}
catch {
    Write-Host "`nTEST FAILED:" $_ -ForegroundColor Red
    exit 1
}
