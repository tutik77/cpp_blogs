#include "AuthFilter.h"
#include <drogon/drogon.h>
#include <jwt-cpp/jwt.h>
#include <json/value.h>
#include <json/writer.h>

void AuthFilter::doFilter(const HttpRequestPtr &req,
                          FilterCallback &&fcb,
                          FilterChainCallback &&fccb) {
  auto authHeader = req->getHeader("Authorization");
  
  if (authHeader.empty()) {
    Json::Value response;
    response["error"] = "Missing Authorization header";
    auto resp = HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(k401Unauthorized);
    fcb(resp);
    return;
  }

  if (authHeader.substr(0, 7) != "Bearer ") {
    Json::Value response;
    response["error"] = "Invalid Authorization format";
    auto resp = HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(k401Unauthorized);
    fcb(resp);
    return;
  }

  std::string token = authHeader.substr(7);
  auto config = drogon::app().getCustomConfig();
  std::string secret = config.get("jwt_secret", "secret").asString();

  try {
    auto verifier = jwt::verify()
                        .allow_algorithm(jwt::algorithm::hs256{secret})
                        .with_issuer("auth0");
    
    auto decoded = jwt::decode(token);
    verifier.verify(decoded);

    auto user_id_claim = decoded.get_payload_claim("user_id");
    if (user_id_claim.as_string().empty()) {
      Json::Value response;
      response["error"] = "Invalid token payload";
      auto resp = HttpResponse::newHttpJsonResponse(response);
      resp->setStatusCode(k401Unauthorized);
      fcb(resp);
      return;
    }

    // Store user_id as int64_t so that subsequent get<int64_t>("user_id")
    // calls in controllers match the exact type and don't trigger
    // Attribute's "Bad type" runtime error.
    int64_t userId = static_cast<int64_t>(std::stoll(user_id_claim.as_string()));
    req->attributes()->insert("user_id", userId);
    
    fccb();
  } catch (const std::exception &e) {
    Json::Value response;
    response["error"] = "Invalid or expired token";
    auto resp = HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(k401Unauthorized);
    fcb(resp);
  }
}
