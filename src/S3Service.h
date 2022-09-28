// This is a personal academic project. Dear PVS-Studio, please check it.

// PVS-Studio Static Code Analyzer for C, C++, C#, and Java:
// https://pvs-studio.com

#ifndef MY_PROJECT_SRC_S3SERVICE_H_
#define MY_PROJECT_SRC_S3SERVICE_H_
#include <filesystem>
#include <fstream>
#include <iostream>
#include <aws/core/Aws.h>
#include <aws/core/auth/AWSCredentialsProvider.h>
#include <aws/core/utils/logging/LogLevel.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/PutObjectRequest.h>
#include "ConfigService.h"
namespace fs = std::filesystem;
using namespace Aws;
class S3Service
{
public:
    static void putObject(string filePath, S3Profile * s3ProfileIn)
    {
        // The Aws::SDKOptions struct contains SDK configuration options.
        // An instance of Aws::SDKOptions is passed to the Aws::InitAPI and
        // Aws::ShutdownAPI methods.  The same instance should be sent to both
        // methods.
        SDKOptions options;
        S3Profile s3_profile;
        s3_profile.accessKeyId = s3ProfileIn->accessKeyId;
        s3_profile.bucketName = s3ProfileIn->bucketName;
        s3_profile.endpoint = s3ProfileIn->endpoint;
        s3_profile.secretKey = s3ProfileIn->secretKey;
        s3_profile.verifySsl = s3ProfileIn->verifySsl;
        //    string filePath = *filepathIn;
        options.loggingOptions.logLevel = Utils::Logging::LogLevel::Debug;
        Auth::AWSCredentials credentials;

        // The AWS SDK for C++ must be initialized by calling Aws::InitAPI.
        InitAPI(options);
        {
            //    S3::S3Client client;
            Aws::Client::ClientConfiguration clientConfig;
            credentials.SetAWSAccessKeyId(Aws::String(s3_profile.accessKeyId));
            credentials.SetAWSSecretKey(Aws::String(s3_profile.secretKey));
            clientConfig.endpointOverride = s3_profile.endpoint;
            clientConfig.verifySSL = s3_profile.verifySsl;
            clientConfig.scheme = Aws::Http::Scheme::HTTP;
            Aws::S3::S3Client client(credentials, clientConfig, Aws::Client::AWSAuthV4Signer::PayloadSigningPolicy::Never, false);

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
            // TODO: Create a file called "my-file.txt" in the local folder where your
            // executables are built to.
            const Aws::String object_name = fs::path(filePath).filename();
            const Aws::String file = filePath;
            // TODO: Set to the AWS Region in which the bucket was created.
            //    const Aws::String region = "us-east-1";
            Aws::S3::Model::PutObjectRequest request;
            //    request.SetBucket(bucket_name);
            //    request.SetKey(object_name);
            request.WithBucket(bucket_name.c_str()).WithKey(object_name.c_str());
            std::shared_ptr<Aws::IOStream> input_data
                = Aws::MakeShared<Aws::FStream>("SampleAllocationTag", file.c_str(), std::ios_base::in | std::ios_base::binary);
            request.SetBody(input_data);

            Aws::S3::Model::PutObjectOutcome outcome1 = client.PutObject(request);

            if (outcome1.IsSuccess())
            {
                std::cout << "Added object '" << object_name << "' to bucket '" << bucket_name << "'.";
            }
            else
            {
                std::cout << "Error: PutObject: " << outcome1.GetError().GetMessage() << std::endl;
            }
        }

        // Before the application terminates, the SDK must be shut down.
      //  ShutdownAPI(options);
    }
};
#endif // MY_PROJECT_SRC_S3SERVICE_H_
