#!/usr/bin/env bash
set -euo pipefail

BASE_URL="${BASE_URL:-http://localhost:3001}"

echo "Running smoke tests against ${BASE_URL}"

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

body=$(curl -s "${BASE_URL}/users/1/followers") || fail "failed to read body for /users/1/followers"
echo "   Response body: ${body}"

echo "2) GET /feed without Authorization (should be 401)"
status=$(curl -s -o /dev/null -w "%{http_code}" "${BASE_URL}/feed") || fail "request to /feed failed"
echo "   HTTP status: ${status}"
if [ "${status}" -ne 401 ]; then
  fail "expected status 401 for /feed without token, got ${status}"
fi

echo
echo "All smoke tests passed ✔"


