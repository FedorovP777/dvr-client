#include <gtest/gtest.h>
#include "../src/ConfigService.h"
TEST(ConfigTest, BasicAssertions)
{
    ConfigService cs;
    cs.loadConfigFile("tests/config_test.yaml");
    StreamSetting stream = cs.getStreams()[0];
    EXPECT_EQ(stream.input_stream_setting.src, "rtsp://11.11.11.11:554/test");
    EXPECT_EQ(stream.input_stream_setting.name, "name_1");
    EXPECT_EQ(stream.output_stream_settings[0].path, "file-480-%Y-%m-%d-{unixtime}.ts");
    EXPECT_EQ(stream.output_stream_settings[0].bitrate, 5000);
    EXPECT_EQ(stream.output_stream_settings[0].height, 480);
    EXPECT_EQ(stream.output_stream_settings[1].path, "/output/file-unmuxing-%Y-%m-%d-{unixtime}.ts");
    auto s3_profile = cs.getS3Profile("default");
    EXPECT_EQ(s3_profile->name, "default");
    EXPECT_EQ(s3_profile->endpoint, "192.168.2.14:9000");
    EXPECT_EQ(s3_profile->accessKeyId, "access_key_id");
    EXPECT_EQ(s3_profile->secretKey, "secret_key");
    EXPECT_EQ(s3_profile->bucketName, "my-first-bucket");
    EXPECT_EQ(s3_profile->verifySsl, false);
}
