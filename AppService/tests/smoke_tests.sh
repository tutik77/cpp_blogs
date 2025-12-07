#!/usr/bin/env bash
set -euo pipefail

BASE_URL="${BASE_URL:-http://localhost:3001}"
AUTH_URL="${AUTH_URL:-http://localhost:3000}"

echo "Running smoke tests against ${BASE_URL} and ${AUTH_URL}"

fail() {
  echo "SMOKE TEST FAILED: $1"
  exit 1
}

echo "1) GET /users/1/followers (public, without auth)"
status=$(curl -s -o /dev/null -w "%{http_code}" "${BASE_URL}/users/1/followers") || fail "request to /users/1/followers failed"
echo "   HTTP status: ${status}"
if [ "${status}" -ne 200 ]; then
  fail "expected status 200 for /users/1/followers, got ${status}"
fi

echo "2) GET /feed without Authorization (should be 401)"
status=$(curl -s -o /dev/null -w "%{http_code}" "${BASE_URL}/feed") || fail "request to /feed failed"
echo "   HTTP status: ${status}"
if [ "${status}" -ne 401 ]; then
  fail "expected status 401 for /feed without token, got ${status}"
fi

echo "3) AuthService healthcheck"
status=$(curl -s -o /dev/null -w "%{http_code}" "${AUTH_URL}/v1/Auth/healthcheck") || fail "auth healthcheck failed"
echo "   Auth healthcheck status: ${status}"
if [ "${status}" -ne 200 ]; then
  fail "expected status 200 for auth healthcheck, got ${status}"
fi

echo "4) AppService healthcheck (GET /)"
status=$(curl -s -o /dev/null -w "%{http_code}" "${BASE_URL}/") || fail "app root endpoint failed"
echo "   App root status: ${status}"
if [ "${status}" -ne 200 ] && [ "${status}" -ne 404 ]; then
  fail "expected status 200/404 for app root, got ${status}"
fi

echo "5) GET /posts (public posts list)"
status=$(curl -s -o /dev/null -w "%{http_code}" "${BASE_URL}/posts") || fail "request to /posts failed"
echo "   HTTP status: ${status}"
if [ "${status}" -ne 200 ]; then
  fail "expected status 200 for /posts, got ${status}"
fi

echo "6) POST /posts without auth (should be 401)"
status=$(curl -s -o /dev/null -w "%{http_code}" -X POST "${BASE_URL}/posts" \
  -H "Content-Type: application/json" \
  -d '{"title":"test","content":"test"}') || fail "POST /posts failed"
echo "   HTTP status: ${status}"
if [ "${status}" -ne 401 ]; then
  fail "expected status 401 for POST /posts without token, got ${status}"
fi

echo "7) GET /users/1 (public user info)"
status=$(curl -s -o /dev/null -w "%{http_code}" "${BASE_URL}/users/1") || fail "request to /users/1 failed"
echo "   HTTP status: ${status}"
if [ "${status}" -ne 200 ]; then
  fail "expected status 200 for /users/1, got ${status}"
fi

echo "8) OPTIONS preflight CORS check"
status=$(curl -s -o /dev/null -w "%{http_code}" -X OPTIONS "${BASE_URL}/feed" \
  -H "Origin: http://localhost:8080" \
  -H "Access-Control-Request-Method: GET") || fail "CORS preflight failed"
echo "   CORS status: ${status}"
if [ "${status}" -ne 200 ] && [ "${status}" -ne 204 ]; then
  fail "expected CORS status 200/204, got ${status}"
fi

echo
echo "All smoke tests passed ✔"
