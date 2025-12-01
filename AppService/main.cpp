#include <drogon/drogon.h>

int main() {
  LOG_DEBUG << "Load config file";
  drogon::app().loadConfigFile("config.json");

  // Add CORS support for frontend
  drogon::app().registerPreRoutingAdvice([](const drogon::HttpRequestPtr &req,
                                           drogon::AdviceCallback &&acb,
                                           drogon::AdviceChainCallback &&accb) {
    // Handle OPTIONS preflight requests
    if (req->method() == drogon::HttpMethod::Options) {
      auto resp = drogon::HttpResponse::newHttpResponse();
      resp->addHeader("Access-Control-Allow-Origin", "*");
      resp->addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
      resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
      resp->addHeader("Access-Control-Max-Age", "86400");
      acb(resp);
      return;
    }
    accb();
  });

  // Add CORS headers to all responses
  drogon::app().registerPostHandlingAdvice([](const drogon::HttpRequestPtr &,
                                             const drogon::HttpResponsePtr &resp) {
    resp->addHeader("Access-Control-Allow-Origin", "*");
    resp->addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
  });

  LOG_DEBUG << "running on localhost:3001";
  drogon::app().run();
  return 0;
}
