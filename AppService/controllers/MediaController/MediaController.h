#pragma once

#include <drogon/HttpController.h>

using namespace drogon;

namespace api {
class MediaController : public drogon::HttpController<MediaController> {
public:
  METHOD_LIST_BEGIN
  
  ADD_METHOD_TO(MediaController::uploadMedia, "/media/upload", Post, "AuthFilter");
  ADD_METHOD_TO(MediaController::attachToPost, "/posts/{1}/attach", Post, "AuthFilter");
  
  METHOD_LIST_END

  void uploadMedia(const HttpRequestPtr &req,
                   std::function<void(const HttpResponsePtr &)> &&callback) const;

  void attachToPost(const HttpRequestPtr &req,
                    std::function<void(const HttpResponsePtr &)> &&callback,
                    int64_t postId) const;
};
}
