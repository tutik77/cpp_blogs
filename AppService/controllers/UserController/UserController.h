#pragma once

#include <drogon/HttpController.h>

using namespace drogon;

namespace api {
class UserController : public drogon::HttpController<UserController> {
public:
  METHOD_LIST_BEGIN
  
  ADD_METHOD_TO(UserController::getUser, "/users/{1}", Get, "AuthFilter");
  ADD_METHOD_TO(UserController::updateProfile, "/users/me", Put, "AuthFilter");
  ADD_METHOD_TO(UserController::followUser, "/users/{1}/follow", Post, "AuthFilter");
  ADD_METHOD_TO(UserController::unfollowUser, "/users/{1}/follow", Delete, "AuthFilter");
  ADD_METHOD_TO(UserController::getFollowers, "/users/{1}/followers", Get);
  ADD_METHOD_TO(UserController::getFollowing, "/users/{1}/following", Get);
  
  METHOD_LIST_END

  void getUser(const HttpRequestPtr &req,
               std::function<void(const HttpResponsePtr &)> &&callback,
               int64_t userId) const;

  void updateProfile(const HttpRequestPtr &req,
                     std::function<void(const HttpResponsePtr &)> &&callback) const;

  void followUser(const HttpRequestPtr &req,
                  std::function<void(const HttpResponsePtr &)> &&callback,
                  int64_t userId) const;

  void unfollowUser(const HttpRequestPtr &req,
                    std::function<void(const HttpResponsePtr &)> &&callback,
                    int64_t userId) const;

  void getFollowers(const HttpRequestPtr &req,
                    std::function<void(const HttpResponsePtr &)> &&callback,
                    int64_t userId) const;

  void getFollowing(const HttpRequestPtr &req,
                    std::function<void(const HttpResponsePtr &)> &&callback,
                    int64_t userId) const;
};
}
