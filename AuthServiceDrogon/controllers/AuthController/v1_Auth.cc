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
  Json::Value response, jsonRequest;
  Json::Reader reader;
  auto resp = HttpResponse::newHttpResponse();
  Json::FastWriter fastWriter;

  try {
    bool requestParsing =
        reader.parse(std::string{req->getBody()}, jsonRequest);
    auto db_client = drogon::app().getDbClient();

    if (requestParsing) {
      try {
        // Try to find user
        drogon::orm::Mapper<drogon_model::auth_service::Users> mapper(
            db_client);
        auto dbuser = mapper.findOne(
            drogon::orm::Criteria("login", drogon::orm::CompareOperator::EQ,
                                  jsonRequest.get("login", "None").asString()));

        // create enctyptedPassword
        std::string enctyptedPassword = BCrypt::generateHash(
            jsonRequest.get("password", "None").asString());

        // check if valid username and login
        if (!BCrypt::validatePassword(
                jsonRequest.get("password", "None").asString(),
                dbuser.getValueOfPassword())) {
          response["msg"] = "invalid password";
          resp->setStatusCode(k400BadRequest);
          resp->setContentTypeCode(CT_APPLICATION_JSON);
          resp->setBody(fastWriter.write(response));
          callback(resp);
          return;
        }

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
        callback(resp);

      } catch (const std::exception &e) {
        response["msg"] = "Did not find user with this login";
        resp->setStatusCode(k400BadRequest);
        resp->setContentTypeCode(CT_APPLICATION_JSON);
        resp->setBody(fastWriter.write(response));
        callback(resp);
      }

    } else {
      response["msg"] = "Cannot parse that body";
      resp->setStatusCode(k400BadRequest);
      resp->setContentTypeCode(CT_APPLICATION_JSON);
      resp->setBody(fastWriter.write(response));
      callback(resp);
    }
  } catch (const std::exception &e) {
    response["msg"] = "Server error";
    resp->setStatusCode(drogon::k500InternalServerError);
    resp->setContentTypeCode(CT_APPLICATION_JSON);
    resp->setBody(fastWriter.write(response));
    callback(resp);
  }
}

void Auth::reg(const HttpRequestPtr &req,
               std::function<void(const HttpResponsePtr &)> &&callback) const {

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
      try {
        // If user found it will return that Username already exists

        // Try to find username
        drogon::orm::Mapper<drogon_model::auth_service::Users> mapper(
            db_client);
        auto dbuser = mapper.findOne(
            drogon::orm::Criteria("login", drogon::orm::CompareOperator::EQ,
                                  jsonRequest.get("login", "None").asString()));

        // return response
        response["msg"] = "Login already exist";
        resp->setStatusCode(k400BadRequest);
        resp->setContentTypeCode(CT_APPLICATION_JSON);
        resp->setBody(fastWriter.write(response));
        callback(resp);

      } catch (const std::exception &e) {
        // Username not found then create a new User

        auto user = drogon_model::auth_service::Users(jsonRequest);

        std::string enctyptedPassword =
            BCrypt::generateHash(user.getValueOfPassword());
        user.setPassword(enctyptedPassword);

        // Use mapper to insert the user
        drogon::orm::Mapper<drogon_model::auth_service::Users> mapper(
            db_client);
        
        try {
          mapper.insert(user);
          LOG_DEBUG << "User created!";
        } catch (const drogon::orm::DrogonDbException &ex) {
          LOG_ERROR << "error:" << ex.base().what();
          response["msg"] = "Failed to create user";
          resp->setStatusCode(k500InternalServerError);
          resp->setContentTypeCode(CT_APPLICATION_JSON);
          resp->setBody(fastWriter.write(response));
          callback(resp);
          return;
        }

        resp->setStatusCode(k201Created);
        resp->setContentTypeCode(CT_APPLICATION_JSON);

        auto id = mapper
                      .findOne(drogon::orm::Criteria(
                          "login", drogon::orm::CompareOperator::EQ,
                          user.getValueOfLogin()))
                      .getValueOfId();

        // after successful user creation create jwt token
        auto token =
            jwt::create()
                .set_type("JWS")
                .set_issuer("auth0")
                .set_payload_claim("user_id",
                                   jwt::claim(std::to_string(id)))
                .sign(jwt::algorithm::hs256("secret")); // need to parse secret

        response["token"] = token;
        resp->setBody(fastWriter.write(response));
        LOG_INFO << "User created: " << user.getValueOfName();

        callback(resp);
      }
    } else {
      // Cannot parse body return 401

      LOG_ERROR << "Cannot parse body";

      // return response
      response["msg"] = "Cannot parse that body";
      resp->setStatusCode(k400BadRequest);
      resp->setContentTypeCode(CT_APPLICATION_JSON);
      resp->setBody(fastWriter.write(response));
      callback(resp);
    }
  } catch (const std::exception &e) {
    // Catched error returns 500
    response["msg"] = "Server error";
    resp->setStatusCode(drogon::k500InternalServerError);
    resp->setContentTypeCode(CT_APPLICATION_JSON);
    resp->setBody(fastWriter.write(response));
    callback(resp);
  }
};

void Auth::healthcheck(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) const {
  LOG_DEBUG << "Used heathcheck\n";

  Json::Value res;
  res["status"] = "ok";

  auto resp = HttpResponse::newHttpJsonResponse(res);
  callback(resp);
};
