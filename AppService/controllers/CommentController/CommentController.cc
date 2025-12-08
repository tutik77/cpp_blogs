#include "CommentController.h"
#include <json/value.h>

using namespace api;

void CommentController::getComments(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    int64_t postId) const {

  auto db = drogon::app().getDbClient();

  try {
    auto result =
        db->execSqlSync("SELECT c.id, c.author_user_id, c.text, c.created_at, "
                        "       u.username "
                        "FROM comments c "
                        "LEFT JOIN users u ON u.user_id = c.author_user_id "
                        "WHERE c.post_id = $1 ORDER BY c.created_at ASC",
                        postId);

    Json::Value comments(Json::arrayValue);
    for (const auto &row : result) {
      Json::Value comment;
      comment["id"] = (Json::Int64)row["id"].as<int64_t>();
      comment["post_id"] = (Json::Int64)postId;
      comment["author_user_id"] =
          (Json::Int64)row["author_user_id"].as<int64_t>();

      std::string text = row["text"].as<std::string>();
      comment["text"] = text;
      // Фронтенд ожидает поле `content`
      comment["content"] = text;

      comment["created_at"] = row["created_at"].as<std::string>();

      if (!row["username"].isNull()) {
        comment["author_username"] = row["username"].as<std::string>();
      } else {
        comment["author_username"] = "";
      }

      comments.append(comment);
    }

    Json::Value response;
    response["comments"] = comments;

    auto resp = HttpResponse::newHttpJsonResponse(response);
    callback(resp);

  } catch (const std::exception &e) {
    LOG_ERROR << "Error getting comments: " << e.what();
    Json::Value response;
    response["error"] = "Internal server error";
    auto resp = HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(k500InternalServerError);
    callback(resp);
  }
}

void CommentController::createComment(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    int64_t postId) const {

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

    auto result =
        db->execSqlSync("INSERT INTO comments (post_id, author_user_id, text) "
                        "VALUES ($1, $2, $3) RETURNING id, created_at",
                        postId, userId, text);

    // Попробуем получить username автора, если он есть в таблице users
    std::string authorUsername;
    try {
      auto userRow = db->execSqlSync(
          "SELECT username FROM users WHERE user_id = $1", userId);
      if (!userRow.empty() && !userRow[0]["username"].isNull()) {
        authorUsername = userRow[0]["username"].as<std::string>();
      }
    } catch (const std::exception &e) {
      // Не критично, если не нашли профиль
      LOG_DEBUG << "No user profile for comment author: " << e.what();
    }

    Json::Value response;
    response["id"] = (Json::Int64)result[0]["id"].as<int64_t>();
    response["post_id"] = (Json::Int64)postId;
    response["author_user_id"] = (Json::Int64)userId;
    response["text"] = text;
    response["content"] = text;
    response["created_at"] = result[0]["created_at"].as<std::string>();
    response["author_username"] = authorUsername;

    auto resp = HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(k201Created);
    callback(resp);

  } catch (const std::exception &e) {
    LOG_ERROR << "Error creating comment: " << e.what();
    Json::Value response;
    response["error"] = "Internal server error";
    auto resp = HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(k500InternalServerError);
    callback(resp);
  }
}

void CommentController::deleteComment(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    int64_t commentId) const {

  auto userId = req->attributes()->get<int64_t>("user_id");
  auto db = drogon::app().getDbClient();

  try {
    auto commentResult = db->execSqlSync(
        "SELECT author_user_id FROM comments WHERE id = $1", commentId);

    if (commentResult.empty()) {
      Json::Value response;
      response["error"] = "Comment not found";
      auto resp = HttpResponse::newHttpJsonResponse(response);
      resp->setStatusCode(k404NotFound);
      callback(resp);
      return;
    }

    if (commentResult[0]["author_user_id"].as<int64_t>() != userId) {
      Json::Value response;
      response["error"] = "Forbidden";
      auto resp = HttpResponse::newHttpJsonResponse(response);
      resp->setStatusCode(k403Forbidden);
      callback(resp);
      return;
    }

    db->execSqlSync("DELETE FROM comments WHERE id = $1", commentId);

    Json::Value response;
    response["success"] = true;
    auto resp = HttpResponse::newHttpJsonResponse(response);
    callback(resp);

  } catch (const std::exception &e) {
    LOG_ERROR << "Error deleting comment: " << e.what();
    Json::Value response;
    response["error"] = "Internal server error";
    auto resp = HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(k500InternalServerError);
    callback(resp);
  }
}
