#include "UserController.h"
#include <json/value.h>
#include <drogon/orm/Mapper.h>

using namespace api;

void UserController::getUser(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    int64_t userId) const {
  
  auto db = drogon::app().getDbClient();
  
  try {
    auto result = db->execSqlSync(
      "SELECT user_id, username, display_name, bio, avatar_path, created_at "
      "FROM users WHERE user_id = $1",
      userId
    );

    if (result.empty()) {
      Json::Value response;
      response["error"] = "User not found";
      auto resp = HttpResponse::newHttpJsonResponse(response);
      resp->setStatusCode(k404NotFound);
      callback(resp);
      return;
    }

    auto row = result[0];
    Json::Value user;
    user["user_id"] = (Json::Int64)row["user_id"].as<int64_t>();
    user["username"] = row["username"].as<std::string>();
    user["display_name"] = row["display_name"].isNull() ? "" : row["display_name"].as<std::string>();
    user["bio"] = row["bio"].isNull() ? "" : row["bio"].as<std::string>();
    user["avatar_path"] = row["avatar_path"].isNull() ? "" : row["avatar_path"].as<std::string>();
    user["created_at"] = row["created_at"].as<std::string>();

    auto followersCount = db->execSqlSync(
      "SELECT COUNT(*) as count FROM follows WHERE following_user_id = $1",
      userId
    );
    user["followers_count"] = (Json::Int64)followersCount[0]["count"].as<int64_t>();

    auto followingCount = db->execSqlSync(
      "SELECT COUNT(*) as count FROM follows WHERE follower_user_id = $1",
      userId
    );
    user["following_count"] = (Json::Int64)followingCount[0]["count"].as<int64_t>();

    auto resp = HttpResponse::newHttpJsonResponse(user);
    callback(resp);

  } catch (const std::exception &e) {
    LOG_ERROR << "Error getting user: " << e.what();
    Json::Value response;
    response["error"] = "Internal server error";
    auto resp = HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(k500InternalServerError);
    callback(resp);
  }
}

void UserController::updateProfile(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) const {
  
  auto userId = req->attributes()->get<int64_t>("user_id");
  auto json = req->getJsonObject();

  if (!json) {
    Json::Value response;
    response["error"] = "Invalid JSON";
    auto resp = HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(k400BadRequest);
    callback(resp);
    return;
  }

  auto db = drogon::app().getDbClient();

  try {
    auto existingUser = db->execSqlSync(
      "SELECT id FROM users WHERE user_id = $1",
      userId
    );

    if (existingUser.empty()) {
      std::string username = json->get("username", "").asString();
      std::string displayName = json->get("display_name", "").asString();
      std::string bio = json->get("bio", "").asString();
      
      db->execSqlSync(
        "INSERT INTO users (user_id, username, display_name, bio) VALUES ($1, $2, $3, $4)",
        userId, username, displayName, bio
      );
    } else {
      std::vector<std::string> updates;
      std::vector<std::string> params;
      int paramIndex = 1;

      if (json->isMember("display_name")) {
        updates.push_back("display_name = $" + std::to_string(paramIndex++));
        params.push_back((*json)["display_name"].asString());
      }
      if (json->isMember("bio")) {
        updates.push_back("bio = $" + std::to_string(paramIndex++));
        params.push_back((*json)["bio"].asString());
      }
      if (json->isMember("avatar_path")) {
        updates.push_back("avatar_path = $" + std::to_string(paramIndex++));
        params.push_back((*json)["avatar_path"].asString());
      }

      if (!updates.empty()) {
        std::string sql = "UPDATE users SET ";
        for (size_t i = 0; i < updates.size(); ++i) {
          sql += updates[i];
          if (i < updates.size() - 1) sql += ", ";
        }
        sql += " WHERE user_id = $" + std::to_string(paramIndex);

        if (params.size() == 1) {
          db->execSqlSync(sql, params[0], userId);
        } else if (params.size() == 2) {
          db->execSqlSync(sql, params[0], params[1], userId);
        } else if (params.size() == 3) {
          db->execSqlSync(sql, params[0], params[1], params[2], userId);
        }
      }
    }

    Json::Value response;
    response["success"] = true;
    auto resp = HttpResponse::newHttpJsonResponse(response);
    callback(resp);

  } catch (const std::exception &e) {
    LOG_ERROR << "Error updating profile: " << e.what();
    Json::Value response;
    response["error"] = "Internal server error";
    auto resp = HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(k500InternalServerError);
    callback(resp);
  }
}

void UserController::followUser(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    int64_t targetUserId) const {
  
  auto currentUserId = req->attributes()->get<int64_t>("user_id");

  if (currentUserId == targetUserId) {
    Json::Value response;
    response["error"] = "Cannot follow yourself";
    auto resp = HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(k400BadRequest);
    callback(resp);
    return;
  }

  auto db = drogon::app().getDbClient();

  try {
    db->execSqlSync(
      "INSERT INTO follows (follower_user_id, following_user_id) VALUES ($1, $2) ON CONFLICT DO NOTHING",
      currentUserId, targetUserId
    );

    Json::Value response;
    response["success"] = true;
    auto resp = HttpResponse::newHttpJsonResponse(response);
    callback(resp);

  } catch (const std::exception &e) {
    LOG_ERROR << "Error following user: " << e.what();
    Json::Value response;
    response["error"] = "Internal server error";
    auto resp = HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(k500InternalServerError);
    callback(resp);
  }
}

void UserController::unfollowUser(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    int64_t targetUserId) const {
  
  auto currentUserId = req->attributes()->get<int64_t>("user_id");
  auto db = drogon::app().getDbClient();

  try {
    db->execSqlSync(
      "DELETE FROM follows WHERE follower_user_id = $1 AND following_user_id = $2",
      currentUserId, targetUserId
    );

    Json::Value response;
    response["success"] = true;
    auto resp = HttpResponse::newHttpJsonResponse(response);
    callback(resp);

  } catch (const std::exception &e) {
    LOG_ERROR << "Error unfollowing user: " << e.what();
    Json::Value response;
    response["error"] = "Internal server error";
    auto resp = HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(k500InternalServerError);
    callback(resp);
  }
}

void UserController::getFollowers(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    int64_t userId) const {
  
  auto db = drogon::app().getDbClient();

  try {
    auto result = db->execSqlSync(
      "SELECT u.user_id, u.username, u.display_name, u.avatar_path "
      "FROM users u "
      "INNER JOIN follows f ON f.follower_user_id = u.user_id "
      "WHERE f.following_user_id = $1 "
      "ORDER BY f.created_at DESC",
      userId
    );

    Json::Value followers(Json::arrayValue);
    for (const auto &row : result) {
      Json::Value user;
      user["user_id"] = (Json::Int64)row["user_id"].as<int64_t>();
      user["username"] = row["username"].as<std::string>();
      user["display_name"] = row["display_name"].isNull() ? "" : row["display_name"].as<std::string>();
      user["avatar_path"] = row["avatar_path"].isNull() ? "" : row["avatar_path"].as<std::string>();
      followers.append(user);
    }

    auto resp = HttpResponse::newHttpJsonResponse(followers);
    callback(resp);

  } catch (const std::exception &e) {
    LOG_ERROR << "Error getting followers: " << e.what();
    Json::Value response;
    response["error"] = "Internal server error";
    auto resp = HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(k500InternalServerError);
    callback(resp);
  }
}

void UserController::getFollowing(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    int64_t userId) const {
  
  auto db = drogon::app().getDbClient();

  try {
    auto result = db->execSqlSync(
      "SELECT u.user_id, u.username, u.display_name, u.avatar_path "
      "FROM users u "
      "INNER JOIN follows f ON f.following_user_id = u.user_id "
      "WHERE f.follower_user_id = $1 "
      "ORDER BY f.created_at DESC",
      userId
    );

    Json::Value following(Json::arrayValue);
    for (const auto &row : result) {
      Json::Value user;
      user["user_id"] = (Json::Int64)row["user_id"].as<int64_t>();
      user["username"] = row["username"].as<std::string>();
      user["display_name"] = row["display_name"].isNull() ? "" : row["display_name"].as<std::string>();
      user["avatar_path"] = row["avatar_path"].isNull() ? "" : row["avatar_path"].as<std::string>();
      following.append(user);
    }

    auto resp = HttpResponse::newHttpJsonResponse(following);
    callback(resp);

  } catch (const std::exception &e) {
    LOG_ERROR << "Error getting following: " << e.what();
    Json::Value response;
    response["error"] = "Internal server error";
    auto resp = HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(k500InternalServerError);
    callback(resp);
  }
}
