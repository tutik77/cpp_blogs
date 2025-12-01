#pragma once

#include <drogon/HttpController.h>

using namespace drogon;

namespace api {
class PostController : public drogon::HttpController<PostController> {
public:
  METHOD_LIST_BEGIN
  
  ADD_METHOD_TO(PostController::createPost, "/posts", Post, "AuthFilter");
  ADD_METHOD_TO(PostController::getPost, "/posts/{1}", Get);
  ADD_METHOD_TO(PostController::updatePost, "/posts/{1}", Put, "AuthFilter");
  ADD_METHOD_TO(PostController::deletePost, "/posts/{1}", Delete, "AuthFilter");
  ADD_METHOD_TO(PostController::getUserPosts, "/users/{1}/posts", Get);
  ADD_METHOD_TO(PostController::likePost, "/posts/{1}/like", Post, "AuthFilter");
  ADD_METHOD_TO(PostController::unlikePost, "/posts/{1}/like", Delete, "AuthFilter");
  ADD_METHOD_TO(PostController::searchPosts, "/posts/search", Get);
  
  METHOD_LIST_END

  void createPost(const HttpRequestPtr &req,
                  std::function<void(const HttpResponsePtr &)> &&callback) const;

  void getPost(const HttpRequestPtr &req,
               std::function<void(const HttpResponsePtr &)> &&callback,
               int64_t postId) const;

  void updatePost(const HttpRequestPtr &req,
                  std::function<void(const HttpResponsePtr &)> &&callback,
                  int64_t postId) const;

  void deletePost(const HttpRequestPtr &req,
                  std::function<void(const HttpResponsePtr &)> &&callback,
                  int64_t postId) const;

  void getUserPosts(const HttpRequestPtr &req,
                    std::function<void(const HttpResponsePtr &)> &&callback,
                    int64_t userId) const;

  void likePost(const HttpRequestPtr &req,
                std::function<void(const HttpResponsePtr &)> &&callback,
                int64_t postId) const;

  void unlikePost(const HttpRequestPtr &req,
                  std::function<void(const HttpResponsePtr &)> &&callback,
                  int64_t postId) const;

  void searchPosts(const HttpRequestPtr &req,
                   std::function<void(const HttpResponsePtr &)> &&callback) const;
};
}
