#include "MediaController.h"
#include <json/value.h>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <vector>

using namespace api;

void MediaController::uploadMedia(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) const {
  
  MultiPartParser fileUpload;
  if (fileUpload.parse(req) != 0) {
    Json::Value response;
    response["error"] = "Invalid multipart data";
    auto resp = HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(k400BadRequest);
    callback(resp);
    return;
  }

  auto files = fileUpload.getFiles();
  if (files.empty()) {
    Json::Value response;
    response["error"] = "No file uploaded";
    auto resp = HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(k400BadRequest);
    callback(resp);
    return;
  }

  auto &file = files[0];
  // file.getFileType() возвращает enum FileType, он нам здесь не нужен,
  // нам достаточно расширения файла для проверки.
  auto fileExtensionView = file.getFileExtension();
  std::string fileExtension(fileExtensionView.data(), fileExtensionView.size());
  
  std::vector<std::string> allowedImages = {"jpg", "jpeg", "png", "gif"};
  std::vector<std::string> allowedVideos = {"mp4", "webm", "mov"};
  
  std::string mediaType;
  bool isAllowed = false;
  
  for (const auto &ext : allowedImages) {
    if (fileExtension == ext) {
      mediaType = "photo";
      isAllowed = true;
      break;
    }
  }
  
  if (!isAllowed) {
    for (const auto &ext : allowedVideos) {
      if (fileExtension == ext) {
        mediaType = "video";
        isAllowed = true;
        break;
      }
    }
  }

  if (!isAllowed) {
    Json::Value response;
    response["error"] = "Invalid file type";
    auto resp = HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(k400BadRequest);
    callback(resp);
    return;
  }

  try {
    std::filesystem::create_directories("uploads");
    
    std::string timestamp = std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
    std::string filename = timestamp + "_" + file.getFileName();
    std::string filepath = "uploads/" + filename;
    
    // saveAs без префикса upload_path, чтобы не получить uploads/uploads
    file.saveAs(filename);

    Json::Value response;
    response["file_path"] = filepath;
    response["type"] = mediaType;
    response["filename"] = filename;
    
    auto resp = HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(k201Created);
    callback(resp);

  } catch (const std::exception &e) {
    LOG_ERROR << "Error uploading file: " << e.what();
    Json::Value response;
    response["error"] = "File upload failed";
    auto resp = HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(k500InternalServerError);
    callback(resp);
  }
}

void MediaController::attachToPost(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    int64_t postId) const {
  
  auto userId = req->attributes()->get<int64_t>("user_id");
  auto json = req->getJsonObject();

  if (!json || !json->isMember("file_path") || !json->isMember("type")) {
    Json::Value response;
    response["error"] = "Missing required fields: file_path, type";
    auto resp = HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(k400BadRequest);
    callback(resp);
    return;
  }

  std::string filePath = (*json)["file_path"].asString();
  std::string type = (*json)["type"].asString();

  auto db = drogon::app().getDbClient();

  try {
    auto postResult = db->execSqlSync(
      "SELECT author_user_id FROM posts WHERE id = $1",
      postId
    );

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

    auto result = db->execSqlSync(
      "INSERT INTO attachments (post_id, type, file_path) VALUES ($1, $2, $3) RETURNING id, created_at",
      postId, type, filePath
    );

    Json::Value response;
    response["id"] = (Json::Int64)result[0]["id"].as<int64_t>();
    response["post_id"] = (Json::Int64)postId;
    response["type"] = type;
    response["file_path"] = filePath;
    response["created_at"] = result[0]["created_at"].as<std::string>();

    auto resp = HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(k201Created);
    callback(resp);

  } catch (const std::exception &e) {
    LOG_ERROR << "Error attaching media: " << e.what();
    Json::Value response;
    response["error"] = "Internal server error";
    auto resp = HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(k500InternalServerError);
    callback(resp);
  }
}
