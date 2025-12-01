#pragma once

#include <drogon/HttpController.h>

using namespace drogon;

namespace api {
class FeedController : public drogon::HttpController<FeedController> {
public:
  METHOD_LIST_BEGIN
  
  ADD_METHOD_TO(FeedController::getFeed, "/feed", Get, "AuthFilter");
  
  METHOD_LIST_END

  void getFeed(const HttpRequestPtr &req,
               std::function<void(const HttpResponsePtr &)> &&callback) const;
};
}
