//
// Created by User on 09.10.2022.
//

#ifndef CLANG_TIDY_NOTIFICATIONSERVICE_H
#define CLANG_TIDY_NOTIFICATIONSERVICE_H
#include "LibuvCurlCpp.h"
#include "json.hpp"
using json = nlohmann::json;
struct NotificationContext
{
    ResultUploadFile * result_upload_file;
    S3Profile * s3_profile;
    string dest_name;
    string camera_name;
    NotificationEndpoint * notification_endpoint;
};
class NotificationService
{
public:
    static void successUploadFile(NotificationContext * notification_context)
    {
        LibuvCurlCpp::request_options options;
        std::unordered_map<std::string, std::string> headers;

        headers["Accept"] = "*";
        headers["Content-Type"] = "application/json; charset=utf-8";
        headers["Authorization"] = fmt::format("\"Bearer: {}\"", notification_context->notification_endpoint->access_key);
        options["method"] = "POST";
        options["url"] = notification_context->notification_endpoint->url;
        options["headers"] = headers;

        json j;
        j[0]["type"] = "success_upload";
        j[0]["message"]["size"] = notification_context->result_upload_file->size;
        j[0]["message"]["dst"] = notification_context->result_upload_file->dst;
        j[0]["message"]["dst_name"] = notification_context->dest_name;
        j[0]["message"]["camera_name"] = notification_context->camera_name;
        options["postfields"] = j.dump();
        cout << j.dump() << endl;

        LibuvCurlCpp::LibuvCurlCpp::request(options, [](string body, string header, int result, int statusCode) {
            std::cout << "Response code:" << statusCode << std::endl;
        });
    }
};
#endif //CLANG_TIDY_NOTIFICATIONSERVICE_H
