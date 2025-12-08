#include "PostController.h"
#include <json/value.h>

using namespace api;

void PostController::createPost(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) const {

  auto userId = req->attributes()->get<int64_t>("user_id");
  auto json = req->getJsonObject();

  // Accept both legacy "text" field and frontend "content" field
  if (!json || (!json->isMember("text") && !json->isMember("content"))) {
    Json::Value response;
    response["error"] = "Missing required field: text or content";
    auto resp = HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(k400BadRequest);
    callback(resp);
    return;
  }

  std::string text;
  if (json->isMember("text")) {
    text = (*json)["text"].asString();

  } else {
    text = (*json)["content"].asString();
  }
  std::string visibility = json->get("visibility", "public").asString();

  auto db = drogon::app().getDbClient();

  try {
    auto result = db->execSqlSync(
        "INSERT INTO posts (author_user_id, text, visibility) VALUES ($1, $2, "
        "$3) RETURNING id, created_at, updated_at",
        userId, text, visibility);

    Json::Value response;
    response["id"] = (Json::Int64)result[0]["id"].as<int64_t>();
    response["author_user_id"] = (Json::Int64)userId;
    response["text"] = text;
    response["visibility"] = visibility;
    response["created_at"] = result[0]["created_at"].as<std::string>();
    response["updated_at"] = result[0]["updated_at"].as<std::string>();
    response["attachments"] = Json::Value(Json::arrayValue);
    response["likes_count"] = 0;
    response["comments_count"] = 0;
    response["is_liked"] = false;

    auto resp = HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(k201Created);
    callback(resp);

  } catch (const std::exception &e) {
    LOG_ERROR << "Error creating post: " << e.what();
    Json::Value response;
    response["error"] = "Internal server error";
    auto resp = HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(k500InternalServerError);
    callback(resp);
  }
}

void PostController::getPost(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    int64_t postId) const {

  auto db = drogon::app().getDbClient();
  int64_t currentUserId = 0;
  bool hasCurrentUser = false;

  try {
    currentUserId = req->attributes()->get<int64_t>("user_id");
    hasCurrentUser = true;
  } catch (...) {
  }

  try {
    auto postResult =
        db->execSqlSync("SELECT id, author_user_id, text, visibility, "
                        "created_at, updated_at FROM posts WHERE id = $1",
                        postId);

    if (postResult.empty()) {
      Json::Value response;
      response["error"] = "Post not found";
      auto resp = HttpResponse::newHttpJsonResponse(response);
      resp->setStatusCode(k404NotFound);
      callback(resp);
      return;
    }

    auto row = postResult[0];
    Json::Value post;
    post["id"] = (Json::Int64)row["id"].as<int64_t>();
    post["author_user_id"] = (Json::Int64)row["author_user_id"].as<int64_t>();
    post["text"] = row["text"].as<std::string>();
    post["visibility"] = row["visibility"].as<std::string>();
    post["created_at"] = row["created_at"].as<std::string>();
    post["updated_at"] = row["updated_at"].as<std::string>();

    auto attachmentsResult = db->execSqlSync(
        "SELECT id, type, file_path FROM attachments WHERE post_id = $1",
        postId);

    Json::Value attachments(Json::arrayValue);
    for (const auto &attRow : attachmentsResult) {
      Json::Value attachment;
      attachment["id"] = (Json::Int64)attRow["id"].as<int64_t>();
      attachment["type"] = attRow["type"].as<std::string>();
      attachment["file_path"] = attRow["file_path"].as<std::string>();
      attachments.append(attachment);
    }
    post["attachments"] = attachments;

    auto likesResult =
        hasCurrentUser
            ? db->execSqlSync("SELECT COUNT(*) as count, "
                              "       SUM(CASE WHEN user_id = $1 THEN 1 ELSE 0 "
                              "END) as liked_by_me "
                              "FROM likes WHERE post_id = $2",
                              currentUserId, postId)
            : db->execSqlSync("SELECT COUNT(*) as count, 0 as liked_by_me "
                              "FROM likes WHERE post_id = $1",
                              postId);
    post["likes_count"] = (Json::Int64)likesResult[0]["count"].as<int64_t>();
    int64_t likedByMe = likesResult[0]["liked_by_me"].isNull()
                            ? 0
                            : likesResult[0]["liked_by_me"].as<int64_t>();
    post["is_liked"] = likedByMe > 0;

    auto resp = HttpResponse::newHttpJsonResponse(post);
    callback(resp);

  } catch (const std::exception &e) {
    LOG_ERROR << "Error getting post: " << e.what();
    Json::Value response;
    response["error"] = "Internal server error";
    auto resp = HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(k500InternalServerError);
    callback(resp);
  }
}

void PostController::updatePost(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    int64_t postId) const {

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
    auto postResult = db->execSqlSync(
        "SELECT author_user_id FROM posts WHERE id = $1", postId);

    if (postResult.empty()) {
      Json::Value response;
      response["error"] = "Post not found";
      auto resp = HttpResponse::newHttpJsonResponse(response);
      resp->setStatusCode(k404NotFound);
      callback(resp);
      return;
    }

    if (postResult[0]["author_user_id"].as<int64_t>() != userId) {
      Json::Value response;
      response["error"] = "Forbidden";
      auto resp = HttpResponse::newHttpJsonResponse(response);
      resp->setStatusCode(k403Forbidden);
      callback(resp);
      return;
    }

    std::vector<std::string> updates;
    std::vector<std::string> params;
    int paramIndex = 1;

    if (json->isMember("text")) {
      updates.push_back("text = $" + std::to_string(paramIndex++));
      params.push_back((*json)["text"].asString());
    }
    if (json->isMember("visibility")) {
      updates.push_back("visibility = $" + std::to_string(paramIndex++));
      params.push_back((*json)["visibility"].asString());
    }

    if (!updates.empty()) {
      updates.push_back("updated_at = now()");

      std::string sql = "UPDATE posts SET ";
      for (size_t i = 0; i < updates.size(); ++i) {
        sql += updates[i];
        if (i < updates.size() - 1)
          sql += ", ";
      }
      sql += " WHERE id = $" + std::to_string(paramIndex);

      if (params.size() == 1) {
        db->execSqlSync(sql, params[0], postId);
      } else if (params.size() == 2) {
        db->execSqlSync(sql, params[0], params[1], postId);
      }
    }

    Json::Value response;
    response["success"] = true;
    auto resp = HttpResponse::newHttpJsonResponse(response);
    callback(resp);

  } catch (const std::exception &e) {
    LOG_ERROR << "Error updating post: " << e.what();
    Json::Value response;
    response["error"] = "Internal server error";
    auto resp = HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(k500InternalServerError);
    callback(resp);
  }
}

void PostController::deletePost(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    int64_t postId) const {

  auto userId = req->attributes()->get<int64_t>("user_id");
  auto db = drogon::app().getDbClient();

  try {
    auto postResult = db->execSqlSync(
        "SELECT author_user_id FROM posts WHERE id = $1", postId);

    if (postResult.empty()) {
      Json::Value response;
      response["error"] = "Post not found";
      auto resp = HttpResponse::newHttpJsonResponse(response);
      resp->setStatusCode(k404NotFound);
      callback(resp);
      return;
    }

    if (postResult[0]["author_user_id"].as<int64_t>() != userId) {
      Json::Value response;
      response["error"] = "Forbidden";
      auto resp = HttpResponse::newHttpJsonResponse(response);
      resp->setStatusCode(k403Forbidden);
      callback(resp);
      return;
    }

    db->execSqlSync("DELETE FROM attachments WHERE post_id = $1", postId);
    db->execSqlSync("DELETE FROM likes WHERE post_id = $1", postId);
    db->execSqlSync("DELETE FROM comments WHERE post_id = $1", postId);
    db->execSqlSync("DELETE FROM posts WHERE id = $1", postId);

    Json::Value response;
    response["success"] = true;
    auto resp = HttpResponse::newHttpJsonResponse(response);
    callback(resp);

  } catch (const std::exception &e) {
    LOG_ERROR << "Error deleting post: " << e.what();
    Json::Value response;
    response["error"] = "Internal server error";
    auto resp = HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(k500InternalServerError);
    callback(resp);
  }
}

void PostController::getUserPosts(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    int64_t userId) const {

  auto db = drogon::app().getDbClient();
  int64_t currentUserId = 0;
  bool hasCurrentUser = false;

  try {
    currentUserId = req->attributes()->get<int64_t>("user_id");
    hasCurrentUser = true;
  } catch (...) {
  }

  try {
    auto result =
        db->execSqlSync("SELECT p.id, p.author_user_id, p.text, p.visibility, "
                        "p.created_at, p.updated_at, "
                        "       u.username, u.avatar_path "
                        "FROM posts p "
                        "LEFT JOIN users u ON u.user_id = p.author_user_id "
                        "WHERE p.author_user_id = $1 "
                        "ORDER BY p.created_at DESC",
                        userId);

    Json::Value posts(Json::arrayValue);
    for (const auto &row : result) {
      Json::Value post;
      post["id"] = (Json::Int64)row["id"].as<int64_t>();
      post["author_user_id"] = (Json::Int64)row["author_user_id"].as<int64_t>();
      post["text"] = row["text"].as<std::string>();
      post["visibility"] = row["visibility"].as<std::string>();
      post["created_at"] = row["created_at"].as<std::string>();
      post["updated_at"] = row["updated_at"].as<std::string>();

      if (!row["username"].isNull()) {
        post["author_username"] = row["username"].as<std::string>();
      } else {
        post["author_username"] = "";
      }
      if (!row["avatar_path"].isNull()) {
        post["author_avatar_path"] = row["avatar_path"].as<std::string>();
      } else {
        post["author_avatar_path"] = "";
      }

      int64_t postId = row["id"].as<int64_t>();
      auto likesResult =
          hasCurrentUser
              ? db->execSqlSync("SELECT COUNT(*) as count, "
                                "       SUM(CASE WHEN user_id = $1 THEN 1 ELSE "
                                "0 END) as liked_by_me "
                                "FROM likes WHERE post_id = $2",
                                currentUserId, postId)
              : db->execSqlSync("SELECT COUNT(*) as count, 0 as liked_by_me "
                                "FROM likes WHERE post_id = $1",
                                postId);
      post["likes_count"] = (Json::Int64)likesResult[0]["count"].as<int64_t>();
      int64_t likedByMe = likesResult[0]["liked_by_me"].isNull()
                              ? 0
                              : likesResult[0]["liked_by_me"].as<int64_t>();
      post["is_liked"] = likedByMe > 0;

      auto commentsResult = db->execSqlSync(
          "SELECT COUNT(*) as count FROM comments WHERE post_id = $1", postId);
      post["comments_count"] =
          (Json::Int64)commentsResult[0]["count"].as<int64_t>();

      auto attachmentsResult = db->execSqlSync(
          "SELECT id, type, file_path FROM attachments WHERE post_id = $1",
          postId);

      Json::Value attachments(Json::arrayValue);
      for (const auto &attRow : attachmentsResult) {
        Json::Value attachment;
        attachment["id"] = (Json::Int64)attRow["id"].as<int64_t>();
        attachment["type"] = attRow["type"].as<std::string>();
        attachment["file_path"] = attRow["file_path"].as<std::string>();
        attachments.append(attachment);
      }
      post["attachments"] = attachments;

      posts.append(post);
    }

    auto resp = HttpResponse::newHttpJsonResponse(posts);
    callback(resp);

  } catch (const std::exception &e) {
    LOG_ERROR << "Error getting user posts: " << e.what();
    Json::Value response;
    response["error"] = "Internal server error";
    auto resp = HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(k500InternalServerError);
    callback(resp);
  }
}

void PostController::likePost(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    int64_t postId) const {

  auto userId = req->attributes()->get<int64_t>("user_id");
  auto db = drogon::app().getDbClient();

  try {
    auto postResult =
        db->execSqlSync("SELECT id FROM posts WHERE id = $1", postId);

    if (postResult.empty()) {
      Json::Value response;
      response["error"] = "Post not found";
      auto resp = HttpResponse::newHttpJsonResponse(response);
      resp->setStatusCode(k404NotFound);
      callback(resp);
      return;
    }

    db->execSqlSync("INSERT INTO likes (post_id, user_id) VALUES ($1, $2) ON "
                    "CONFLICT DO NOTHING",
                    postId, userId);

    Json::Value response;
    response["success"] = true;
    auto resp = HttpResponse::newHttpJsonResponse(response);
    callback(resp);

  } catch (const std::exception &e) {
    LOG_ERROR << "Error liking post: " << e.what();
    Json::Value response;
    response["error"] = "Internal server error";
    auto resp = HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(k500InternalServerError);
    callback(resp);
  }
}

void PostController::unlikePost(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    int64_t postId) const {

  auto userId = req->attributes()->get<int64_t>("user_id");
  auto db = drogon::app().getDbClient();

  try {
    db->execSqlSync("DELETE FROM likes WHERE post_id = $1 AND user_id = $2",
                    postId, userId);

    Json::Value response;
    response["success"] = true;
    auto resp = HttpResponse::newHttpJsonResponse(response);
    callback(resp);

  } catch (const std::exception &e) {
    LOG_ERROR << "Error unliking post: " << e.what();
    Json::Value response;
    response["error"] = "Internal server error";
    auto resp = HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(k500InternalServerError);
    callback(resp);
  }
}

void PostController::searchPosts(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) const {

  auto params = req->getParameters();
  std::string query;
  int offset = 0;
  int limit = 20;

  auto it = params.find("q");
  if (it != params.end()) {
    query = it->second;
  }

  if (query.empty()) {
    Json::Value response;
    response["error"] = "Missing query parameter q";
    auto resp = HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(k400BadRequest);
    callback(resp);
    return;
  }

  if (params.find("offset") != params.end()) {
    offset = std::stoi(params.at("offset"));
  }
  if (params.find("limit") != params.end()) {
    limit = std::stoi(params.at("limit"));
    if (limit > 100)
      limit = 100;
  }

  auto db = drogon::app().getDbClient();
  int64_t currentUserId = 0;
  bool hasCurrentUser = false;

  try {
    currentUserId = req->attributes()->get<int64_t>("user_id");
    hasCurrentUser = true;
  } catch (...) {
  }

  try {
    std::string sql =
        "SELECT p.id, p.author_user_id, p.text, p.visibility, p.created_at, "
        "p.updated_at, "
        "       u.username, u.avatar_path, "
        "       ts_rank(to_tsvector('russian', coalesce(p.text, '')), "
        "               websearch_to_tsquery('russian', $2)) as rank, "
        "       CASE WHEN p.author_user_id IN ( "
        "            SELECT following_user_id FROM follows WHERE "
        "follower_user_id = $1 "
        "       ) THEN 0 ELSE 1 END AS follow_priority "
        "FROM posts p "
        "LEFT JOIN users u ON u.user_id = p.author_user_id "
        "WHERE p.visibility = 'public' "
        "  AND to_tsvector('russian', coalesce(p.text, '')) @@ "
        "websearch_to_tsquery('russian', $2) "
        "ORDER BY follow_priority ASC, rank DESC, p.created_at DESC "
        "LIMIT " +
        std::to_string(limit) + " OFFSET " + std::to_string(offset);

    int64_t userForPriority = hasCurrentUser ? currentUserId : 0;

    auto result = db->execSqlSync(sql, userForPriority, query);

    Json::Value posts(Json::arrayValue);
    for (const auto &row : result) {
      Json::Value post;
      post["id"] = (Json::Int64)row["id"].as<int64_t>();
      post["author_user_id"] = (Json::Int64)row["author_user_id"].as<int64_t>();
      post["text"] = row["text"].as<std::string>();
      post["visibility"] = row["visibility"].as<std::string>();
      post["created_at"] = row["created_at"].as<std::string>();
      post["updated_at"] = row["updated_at"].as<std::string>();

      if (!row["username"].isNull()) {
        post["author_username"] = row["username"].as<std::string>();
      } else {
        post["author_username"] = "";
      }
      if (!row["avatar_path"].isNull()) {
        post["author_avatar_path"] = row["avatar_path"].as<std::string>();
      } else {
        post["author_avatar_path"] = "";
      }

      int64_t postId = row["id"].as<int64_t>();
      auto likesResult =
          hasCurrentUser
              ? db->execSqlSync("SELECT COUNT(*) as count, "
                                "       SUM(CASE WHEN user_id = $1 THEN 1 ELSE "
                                "0 END) as liked_by_me "
                                "FROM likes WHERE post_id = $2",
                                currentUserId, postId)
              : db->execSqlSync("SELECT COUNT(*) as count, 0 as liked_by_me "
                                "FROM likes WHERE post_id = $1",
                                postId);
      post["likes_count"] = (Json::Int64)likesResult[0]["count"].as<int64_t>();
      int64_t likedByMe = likesResult[0]["liked_by_me"].isNull()
                              ? 0
                              : likesResult[0]["liked_by_me"].as<int64_t>();
      post["is_liked"] = likedByMe > 0;

      auto commentsResult = db->execSqlSync(
          "SELECT COUNT(*) as count FROM comments WHERE post_id = $1", postId);
      post["comments_count"] =
          (Json::Int64)commentsResult[0]["count"].as<int64_t>();

      auto attachmentsResult = db->execSqlSync(
          "SELECT id, type, file_path FROM attachments WHERE post_id = $1",
          postId);

      Json::Value attachments(Json::arrayValue);
      for (const auto &attRow : attachmentsResult) {
        Json::Value attachment;
        attachment["id"] = (Json::Int64)attRow["id"].as<int64_t>();
        attachment["type"] = attRow["type"].as<std::string>();
        attachment["file_path"] = attRow["file_path"].as<std::string>();
        attachments.append(attachment);
      }
      post["attachments"] = attachments;

      posts.append(post);
    }

    Json::Value response;
    response["posts"] = posts;
    response["offset"] = offset;
    response["limit"] = limit;
    response["has_more"] = posts.size() == (Json::UInt64)limit;
    response["query"] = query;

    auto resp = HttpResponse::newHttpJsonResponse(response);
    callback(resp);

  } catch (const std::exception &e) {
    LOG_ERROR << "Error searching posts: " << e.what();
    Json::Value response;
    response["error"] = "Internal server error";
    auto resp = HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(k500InternalServerError);
    callback(resp);
  }
}
