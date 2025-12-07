#include "v1_Auth.h"
#include "../models/Users.h"
#include "bcrypt/BCrypt.hpp"
#include "drogon/HttpResponse.h"
#include "drogon/HttpTypes.h"
#include "drogon/orm/Criteria.h"
#include "drogon/orm/Mapper.h"
#include "jwt-cpp/traits/kazuho-picojson/defaults.h"
#include "trantor/utils/Logger.h"
#include <exception>
#include <json/value.h>
#include <json/writer.h>
#include <jwt-cpp/jwt.h>

using namespace v1;

void Auth::login(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) const {
    
  LOG_INFO << "=== LOGIN REQUEST STARTED ===";
  LOG_DEBUG << "Request IP: " << req->getPeerAddr().toIp();
  LOG_DEBUG << "Request Method: " << req->getMethodString();
  LOG_DEBUG << "Request Headers: " << req->getHeaders().size() << " headers";
  
  Json::Value response, jsonRequest;
  Json::Reader reader;
  auto resp = HttpResponse::newHttpResponse();
  Json::FastWriter fastWriter;

  try {
    bool requestParsing =
        reader.parse(std::string{req->getBody()}, jsonRequest);
    auto db_client = drogon::app().getDbClient();

    if (requestParsing) {
      std::string login = jsonRequest.get("login", "None").asString();
      LOG_INFO << "Login attempt for user: " << login;
      
      try {
        // Try to find user
        LOG_DEBUG << "Searching for user in database...";
        drogon::orm::Mapper<drogon_model::auth_service::Users> mapper(
            db_client);
        auto dbuser = mapper.findOne(
            drogon::orm::Criteria("login", drogon::orm::CompareOperator::EQ,
                                  login));

        LOG_DEBUG << "User found: ID=" << dbuser.getValueOfId() 
                  << ", Name=" << dbuser.getValueOfName();

        // create enctyptedPassword
        std::string enctyptedPassword = BCrypt::generateHash(
            jsonRequest.get("password", "None").asString());

        // check if valid username and login
        LOG_DEBUG << "Validating password...";
        if (!BCrypt::validatePassword(
                jsonRequest.get("password", "None").asString(),
                dbuser.getValueOfPassword())) {
          LOG_WARN << "Invalid password for user: " << login;
          response["msg"] = "invalid password";
          resp->setStatusCode(k400BadRequest);
          resp->setContentTypeCode(CT_APPLICATION_JSON);
          resp->setBody(fastWriter.write(response));
          LOG_INFO << "Login failed: invalid password";
          callback(resp);
          return;
        }

        LOG_DEBUG << "Password validation successful";
        auto token =
            jwt::create()
                .set_type("JWS")
                .set_issuer("auth0")
                .set_payload_claim("user_id", jwt::claim(std::to_string(
                                                  dbuser.getValueOfId())))
                .sign(jwt::algorithm::hs256("secret")); // need to parse secret
        response["token"] = token;

        resp->setContentTypeCode(CT_APPLICATION_JSON);
        resp->setStatusCode(k200OK);
        resp->setBody(fastWriter.write(response));
        
        LOG_INFO << "Login successful for user: " << login 
                 << " (ID: " << dbuser.getValueOfId() << ")";
        LOG_DEBUG << "JWT token generated: " << token.substr(0, 20) << "...";
        
        callback(resp);

      } catch (const std::exception &e) {
        LOG_WARN << "User not found with login: " << login;
        LOG_DEBUG << "Exception details: " << e.what();
        response["msg"] = "Did not find user with this login";
        resp->setStatusCode(k400BadRequest);
        resp->setContentTypeCode(CT_APPLICATION_JSON);
        resp->setBody(fastWriter.write(response));
        LOG_INFO << "Login failed: user not found";
        callback(resp);
      }

    } else {
      LOG_ERROR << "Failed to parse login request body";
      LOG_DEBUG << "Request body: " << std::string{req->getBody()};
      response["msg"] = "Cannot parse that body";
      resp->setStatusCode(k400BadRequest);
      resp->setContentTypeCode(CT_APPLICATION_JSON);
      resp->setBody(fastWriter.write(response));
      callback(resp);
    }
  } catch (const std::exception &e) {
    LOG_ERROR << "Server error during login: " << e.what();
    response["msg"] = "Server error";
    resp->setStatusCode(drogon::k500InternalServerError);
    resp->setContentTypeCode(CT_APPLICATION_JSON);
    resp->setBody(fastWriter.write(response));
    callback(resp);
  }
  
  LOG_INFO << "=== LOGIN REQUEST COMPLETED ===";
}

void Auth::reg(const HttpRequestPtr &req,
               std::function<void(const HttpResponsePtr &)> &&callback) const {

  LOG_INFO << "=== REGISTRATION REQUEST STARTED ===";
  LOG_DEBUG << "Request IP: " << req->getPeerAddr().toIp();
  LOG_DEBUG << "Request Method: " << req->getMethodString();
  
  Json::Value response, jsonRequest;
  Json::Reader reader;
  Json::FastWriter fastWriter;
  auto resp = HttpResponse::newHttpResponse();
  bool res = true;

  try {
    bool requestParsing =
        reader.parse(std::string{req->getBody()}, jsonRequest);
    auto db_client = drogon::app().getDbClient();

    if (requestParsing) {
      std::string login = jsonRequest.get("login", "None").asString();
      std::string name = jsonRequest.get("name", "None").asString();
      
      LOG_INFO << "Registration attempt - Login: " << login 
               << ", Name: " << name;
      
      try {
        // If user found it will return that Username already exists

        // Try to find username
        LOG_DEBUG << "Checking if login already exists...";
        drogon::orm::Mapper<drogon_model::auth_service::Users> mapper(
            db_client);
        auto dbuser = mapper.findOne(
            drogon::orm::Criteria("login", drogon::orm::CompareOperator::EQ,
                                  login));

        LOG_WARN << "Login already exists: " << login;
        // return response
        response["msg"] = "Login already exist";
        resp->setStatusCode(k400BadRequest);
        resp->setContentTypeCode(CT_APPLICATION_JSON);
        resp->setBody(fastWriter.write(response));
        LOG_INFO << "Registration failed: login already exists";
        callback(resp);

      } catch (const std::exception &e) {
        // Username not found then create a new User
        LOG_DEBUG << "Login not found, proceeding with registration...";
        
        auto user = drogon_model::auth_service::Users(jsonRequest);

        LOG_DEBUG << "Hashing password...";
        std::string enctyptedPassword =
            BCrypt::generateHash(user.getValueOfPassword());
        user.setPassword(enctyptedPassword);

        // Use mapper to insert the user
        drogon::orm::Mapper<drogon_model::auth_service::Users> mapper(
            db_client);
        
        try {
          LOG_DEBUG << "Inserting user into database...";
          mapper.insert(user);
          LOG_DEBUG << "User created in database!";
        } catch (const drogon::orm::DrogonDbException &ex) {
          LOG_ERROR << "Database error during user creation: " << ex.base().what();
          LOG_DEBUG << "SQL error details: " << ex.sql();
          response["msg"] = "Failed to create user";
          resp->setStatusCode(k500InternalServerError);
          resp->setContentTypeCode(CT_APPLICATION_JSON);
          resp->setBody(fastWriter.write(response));
          LOG_ERROR << "Registration failed: database error";
          callback(resp);
          return;
        }

        resp->setStatusCode(k201Created);
        resp->setContentTypeCode(CT_APPLICATION_JSON);

        LOG_DEBUG << "Retrieving user ID...";
        auto id = mapper
                      .findOne(drogon::orm::Criteria(
                          "login", drogon::orm::CompareOperator::EQ,
                          user.getValueOfLogin()))
                      .getValueOfId();

        LOG_DEBUG << "User ID retrieved: " << id;

        // after successful user creation create jwt token
        LOG_DEBUG << "Generating JWT token...";
        auto token =
            jwt::create()
                .set_type("JWS")
                .set_issuer("auth0")
                .set_payload_claim("user_id",
                                   jwt::claim(std::to_string(id)))
                .sign(jwt::algorithm::hs256("secret")); // need to parse secret

        response["token"] = token;
        resp->setBody(fastWriter.write(response));
        
        LOG_INFO << "User registered successfully: " << user.getValueOfName()
                 << " (ID: " << id << ", Login: " << user.getValueOfLogin() << ")";
        LOG_DEBUG << "JWT token generated: " << token.substr(0, 20) << "...";

        callback(resp);
      }
    } else {
      // Cannot parse body return 401
      LOG_ERROR << "Failed to parse registration request body";
      LOG_DEBUG << "Request body: " << std::string{req->getBody()};

      // return response
      response["msg"] = "Cannot parse that body";
      resp->setStatusCode(k400BadRequest);
      resp->setContentTypeCode(CT_APPLICATION_JSON);
      resp->setBody(fastWriter.write(response));
      LOG_ERROR << "Registration failed: cannot parse request body";
      callback(resp);
    }
  } catch (const std::exception &e) {
    // Catched error returns 500
    LOG_ERROR << "Unexpected server error during registration: " << e.what();
    LOG_DEBUG << "Exception type: " << typeid(e).name();
    response["msg"] = "Server error";
    resp->setStatusCode(drogon::k500InternalServerError);
    resp->setContentTypeCode(CT_APPLICATION_JSON);
    resp->setBody(fastWriter.write(response));
    LOG_ERROR << "Registration failed: server error";
    callback(resp);
  }
  
  LOG_INFO << "=== REGISTRATION REQUEST COMPLETED ===";
};

void Auth::healthcheck(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) const {
  
  LOG_DEBUG << "=== HEALTHCHECK REQUEST ===";
  LOG_DEBUG << "Request IP: " << req->getPeerAddr().toIp();
  LOG_DEBUG << "Request Path: " << req->getPath();
  LOG_DEBUG << "Request Headers count: " << req->getHeaders().size();
  
  Json::Value res;
  res["status"] = "ok";
  res["timestamp"] = Json::Value::UInt64(
      std::chrono::duration_cast<std::chrono::seconds>(
          std::chrono::system_clock::now().time_since_epoch())
          .count());
  res["service"] = "auth_service";

  auto resp = HttpResponse::newHttpJsonResponse(res);
  
  LOG_DEBUG << "Healthcheck completed successfully";
  LOG_DEBUG << "Response status: " << resp->getStatusCode();
  
  callback(resp);
};