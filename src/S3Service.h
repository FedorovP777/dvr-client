// This is a personal academic project. Dear PVS-Studio, please check it.

// PVS-Studio Static Code Analyzer for C, C++, C#, and Java:
// https://pvs-studio.com

#ifndef MY_PROJECT_SRC_S3SERVICE_H_
#define MY_PROJECT_SRC_S3SERVICE_H_
#include <filesystem>
#include <fstream>
#include <iostream>
#include <fmt/core.h>
#include <aws/core/Aws.h>
#include <aws/core/auth/AWSCredentialsProvider.h>
#include <aws/core/utils/logging/LogLevel.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/PutObjectRequest.h>
#include "ConfigService.h"
namespace fs = std::filesystem;
using namespace Aws;
struct ResultUploadFile
{
    string filename;
    uintmax_t size;
    string dst;
    string dst_name;
    bool is_success = true;
    string error_message;
};
class S3Service
{
public:
    static ResultUploadFile * putObject(string filePath, const S3Profile * s3Profile)
    {
        // The Aws::SDKOptions struct contains SDK configuration options.
        // An instance of Aws::SDKOptions is passed to the Aws::InitAPI and
        // Aws::ShutdownAPI methods.  The same instance should be sent to both
        // methods.
        S3Profile s3_profile = *s3Profile;
        if (s3_profile.bucketName.empty() || s3_profile.endpoint.empty() || filePath.empty() || !fs::exists(fs::path(filePath)))
        {
            return nullptr;
        }
        string filename = fs::path(filePath).filename();
        string upload_path = filename;
        SDKOptions options;
        options.loggingOptions.logLevel = Utils::Logging::LogLevel::Debug;
        Auth::AWSCredentials credentials;

        // The AWS SDK for C++ must be initialized by calling Aws::InitAPI.
        InitAPI(options);
        {
            Aws::Client::ClientConfiguration client_config;
            credentials.SetAWSAccessKeyId(Aws::String(s3_profile.accessKeyId));
            credentials.SetAWSSecretKey(Aws::String(s3_profile.secretKey));
            client_config.endpointOverride = s3_profile.endpoint;
            client_config.verifySSL = s3_profile.verifySsl;
            client_config.scheme = Aws::Http::Scheme::HTTP;
            Aws::S3::S3Client client(credentials, client_config, Aws::Client::AWSAuthV4Signer::PayloadSigningPolicy::Never, false);

            auto outcome = client.ListBuckets();
            if (outcome.IsSuccess())
            {
                std::cout << "Found " << outcome.GetResult().GetBuckets().size() << " buckets\n";
                for (auto && b : outcome.GetResult().GetBuckets())
                {
                    std::cout << b.GetName() << std::endl;
                }
            }
            else
            {
                std::cout << "Failed with error: " << outcome.GetError() << std::endl;
            }

            const Aws::String bucket_name = s3_profile.bucketName;
            cout << "asdasdasd " << s3_profile.s3_folder << endl;

            if (!s3_profile.s3_folder.empty())
            {
                upload_path = fmt::format("{}{}", s3_profile.s3_folder, filename);
            }

            const Aws::String object_name = upload_path;
            const Aws::String file = filePath;
            // TODO: Set to the AWS Region in which the bucket was created.
            //    const Aws::String region = "us-east-1";
            Aws::S3::Model::PutObjectRequest request;
            request.WithBucket(bucket_name.c_str()).WithKey(object_name.c_str());
            std::shared_ptr<Aws::IOStream> input_data
                = Aws::MakeShared<Aws::FStream>("SampleAllocationTag", file.c_str(), std::ios_base::in | std::ios_base::binary);
            request.SetBody(input_data);

            Aws::S3::Model::PutObjectOutcome put_object_result = client.PutObject(request);

            if (put_object_result.IsSuccess())
            {
                std::cout << "Added object '" << object_name << "' to bucket '" << bucket_name << "'.";
            }
            else
            {
                std::cout << "Error: PutObject: " << put_object_result.GetError().GetMessage() << std::endl;
            }
        }

        // Before the application terminates, the SDK must be shut down.
        //        ShutdownAPI(options);
        return new ResultUploadFile{filePath, fs::file_size(fs::path(filePath)), upload_path};
    }
};
#endif // MY_PROJECT_SRC_S3SERVICE_H_
