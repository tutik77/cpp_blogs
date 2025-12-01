#include "FeedController.h"
#include <json/value.h>

using namespace api;

void FeedController::getFeed(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) const {
  
  auto userId = req->attributes()->get<int64_t>("user_id");

  int offset = 0;
  int limit = 20;
  
  auto params = req->getParameters();
  if (params.find("offset") != params.end()) {
    offset = std::stoi(params.at("offset"));
  }
  if (params.find("limit") != params.end()) {
    limit = std::stoi(params.at("limit"));
    if (limit > 100) limit = 100;
  }

  auto db = drogon::app().getDbClient();

  try {

    std::string sql =
        "SELECT p.id, p.author_user_id, p.text, p.visibility, p.created_at, p.updated_at, "
        "       u.username, u.avatar_path "
        "FROM posts p "
        "LEFT JOIN users u ON u.user_id = p.author_user_id "
        "WHERE p.visibility = 'public' "
        "ORDER BY p.created_at DESC "
        "LIMIT " +
        std::to_string(limit) + " OFFSET " + std::to_string(offset);

    auto result = db->execSqlSync(sql);

    Json::Value feed(Json::arrayValue);
    for (const auto &row : result) {
      Json::Value post;
      post["id"] = (Json::Int64)row["id"].as<int64_t>();
      post["author_user_id"] = (Json::Int64)row["author_user_id"].as<int64_t>();
      post["text"] = row["text"].as<std::string>();
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
      post["visibility"] = row["visibility"].as<std::string>();
      post["created_at"] = row["created_at"].as<std::string>();
      post["updated_at"] = row["updated_at"].as<std::string>();

      int64_t postId = row["id"].as<int64_t>();
      
      auto attachmentsResult = db->execSqlSync(
        "SELECT id, type, file_path FROM attachments WHERE post_id = $1",
        postId
      );

      Json::Value attachments(Json::arrayValue);
      for (const auto &attRow : attachmentsResult) {
        Json::Value attachment;
        attachment["id"] = (Json::Int64)attRow["id"].as<int64_t>();
        attachment["type"] = attRow["type"].as<std::string>();
        attachment["file_path"] = attRow["file_path"].as<std::string>();
        attachments.append(attachment);
      }
      post["attachments"] = attachments;

      auto likesResult = db->execSqlSync(
        "SELECT COUNT(*) as count FROM likes WHERE post_id = $1",
        postId
      );
      post["likes_count"] = (Json::Int64)likesResult[0]["count"].as<int64_t>();

      auto commentsResult = db->execSqlSync(
        "SELECT COUNT(*) as count FROM comments WHERE post_id = $1",
        postId
      );
      post["comments_count"] = (Json::Int64)commentsResult[0]["count"].as<int64_t>();

      feed.append(post);
    }

    Json::Value response;
    response["posts"] = feed;
    response["offset"] = offset;
    response["limit"] = limit;
    response["has_more"] = feed.size() == limit;

    auto resp = HttpResponse::newHttpJsonResponse(response);
    callback(resp);

  } catch (const std::exception &e) {
    LOG_ERROR << "Error getting feed: " << e.what();
    Json::Value response;
    response["error"] = "Internal server error";
    auto resp = HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(k500InternalServerError);
    callback(resp);
  }
}
