#pragma once

#include <drogon/HttpController.h>

using namespace drogon;

namespace api {
class CommentController : public drogon::HttpController<CommentController> {
public:
  METHOD_LIST_BEGIN
  
  ADD_METHOD_TO(CommentController::getComments, "/posts/{1}/comments", Get);
  ADD_METHOD_TO(CommentController::createComment, "/posts/{1}/comments", Post, "AuthFilter");
  ADD_METHOD_TO(CommentController::deleteComment, "/comments/{1}", Delete, "AuthFilter");
  
  METHOD_LIST_END

  void getComments(const HttpRequestPtr &req,
                   std::function<void(const HttpResponsePtr &)> &&callback,
                   int64_t postId) const;

  void createComment(const HttpRequestPtr &req,
                     std::function<void(const HttpResponsePtr &)> &&callback,
                     int64_t postId) const;

  void deleteComment(const HttpRequestPtr &req,
                     std::function<void(const HttpResponsePtr &)> &&callback,
                     int64_t commentId) const;
};
}
