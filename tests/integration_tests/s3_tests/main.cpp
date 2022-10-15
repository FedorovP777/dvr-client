// This is a personal academic project. Dear PVS-Studio, please check it.

// PVS-Studio Static Code Analyzer for C, C++, C#, and Java:
// https://pvs-studio.com
#include <cassert>
#include "../../../src/S3Service.h"
using namespace std;
int main(int /*argc*/, char ** /*argv*/)
{
    S3Profile s3_profile;
    s3_profile.accessKeyId = "minioadmin";
    s3_profile.secretKey = "minioadmin";
    s3_profile.s3_folder = "test";
    s3_profile.bucketName = "vod-bucket";
    s3_profile.endpoint = "192.168.2.91:9000";
    s3_profile.verifySsl = false;
    string demo_file = "tests/integration_tests/s3_tests/file-unmuxing-2022-10-10-1665424500.ts";
    auto * result1 = S3Service::putObject(demo_file, &s3_profile);
    auto * result2 = S3Service::putObject(demo_file, &s3_profile);
    assert(result1 != nullptr);
    assert(result2 != nullptr);
    assert(result1->is_success);
    assert(result2->is_success);
    return 0;
}