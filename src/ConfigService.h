// This is a personal academic project. Dear PVS-Studio, please check it.

// PVS-Studio Static Code Analyzer for C, C++, C#, and Java:
// https://pvs-studio.com
#ifndef LIBAV_TEST_CONFIGSERVICE_H
#define LIBAV_TEST_CONFIGSERVICE_H
#include <cassert>
#include <filesystem>
#include <fmt/core.h>
#include <iostream>
#include <optional>
#include "Options.h"
#include "string"
#include "yaml-cpp/yaml.h"
using namespace std;
struct S3Profile
{
    string name;
    string endpoint;
    string accessKeyId;
    string secretKey;
    string bucketName;
    bool verifySsl = false;
};
struct OutputStreamSetting
{
    string path;
    uint64_t bitrate = 4000000;
    bool rescale = false;
    int width = 0;
    int height = 0;
    optional<int> time_base;
    optional<int> max_b_frames;
    optional<int> gop;
    optional<int> qscale;
    shared_ptr<Options> options;
    int file_duration_sec = 90;
    S3Profile * s3_profile = nullptr;
};
struct InputStreamSetting
{
    string src;
    shared_ptr<Options> options;
    string name;
    bool override_audio_pts = true;
};
struct StreamSetting
{
    InputStreamSetting input_stream_setting;
    vector<OutputStreamSetting> output_stream_settings;
};
class ConfigService
{
private:
    YAML::Node config;

public:
    ConfigService() = default;

    void loadConfigFile(string path)
    {
        const filesystem::path sandbox{path};
        assert(filesystem::exists(path));
        this->config = YAML::LoadFile(path);
    }

    OutputStreamSetting getOutputStreams(YAML::Node node)
    {
        OutputStreamSetting setting;
        setting.path = node["path"].as<string>();

        if (node["width"].IsDefined())
        {
            setting.width = node["width"].as<int>();
            setting.rescale = true;
        }
        if (node["height"].IsDefined())
        {
            setting.height = node["height"].as<int>();
            setting.rescale = true;
        }
        if (node["bitrate"].IsDefined())
        {
            setting.bitrate = node["bitrate"].as<unsigned long>();
        }
        if (node["time_base"].IsDefined())
        {
            setting.time_base = node["time_base"].as<int>();
        }
        if (node["gop"].IsDefined())
        {
            setting.gop = node["gop"].as<int>();
        }
        if (node["max_b_frames"].IsDefined())
        {
            setting.max_b_frames = node["max_b_frames"].as<int>();
        }
        if (node["qscale"].IsDefined())
        {
            setting.qscale = node["qscale"].as<int>();
        }
        if (node["file_duration_sec"].IsDefined())
        {
            setting.file_duration_sec = node["file_duration_sec"].as<int>();
        }
        if (node["s3_target"].IsDefined() && !node["s3_target"].as<string>().empty())
        {
            setting.s3_profile = this->getS3Profile(node["s3_target"].as<string>());
        }

        setting.options = make_shared<Options>();
        for (std::size_t j = 0; j < node["options"].size(); j++)
        {
            const auto optionItem = node["options"][j];
            setting.options->set(optionItem["key"].as<string>().c_str(), optionItem["value"].as<string>().c_str());
        }
        //      result.emplace_back(setting);
        //    }
        return setting;
    }

    /**
     * Получение опций для контекста
     * https://ffmpeg.org/ffmpeg-formats.html#Format-Options
     * @return
     */
    static shared_ptr<Options> getInputContextOptions(YAML::Node node)
    {
        auto options = make_shared<Options>();
        if (!node["options"].IsDefined())
        {
            return options;
        }
        for (std::size_t i = 0; i < node["options"].size(); i++)
        {
            const auto option_item = node["options"][i];
            options->set(option_item["key"].as<string>().c_str(), option_item["value"].as<string>().c_str());
        }
        return options;
    }

    static string getInputStream(const YAML::Node & node)
    {
        stringstream ss;
        ss << node["src"];
        return ss.str();
    }

    S3Profile * getS3Profile(string name)
    {
        if (!this->config["s3_profiles"].IsDefined())
        {
            throw runtime_error("s3_profiles not defined.");
        }
        for (std::size_t i = 0; i < this->config["s3_profiles"].size(); i++)
        {
            auto node = this->config["s3_profiles"][i];
            if (node["name"].IsDefined() && node["name"].as<string>() == name)
            {
                auto * s3profile = new S3Profile();
                s3profile->name = node["name"].as<string>();
                s3profile->endpoint = node["endpoint"].as<string>();
                s3profile->accessKeyId = node["access_key_id"].as<string>();
                s3profile->secretKey = node["secret_key"].as<string>();
                s3profile->bucketName = node["bucket_name"].as<string>();
                s3profile->verifySsl = node["verify_ssl"].as<bool>();
                return s3profile;
            }
        }
        throw runtime_error(fmt::format("{} profile name not defined.", name));
    }

    vector<StreamSetting> getStreams()
    {
        auto result = vector<StreamSetting>();

        for (std::size_t i = 0; i < this->config["streams"].size(); i++)
        {
            StreamSetting ss;

            auto node = this->config["streams"][i];
            if (node["src"].IsDefined())
            {
                ss.input_stream_setting.options = this->getInputContextOptions(node);
                ss.input_stream_setting.src = this->getInputStream(node);
                ss.input_stream_setting.name = node["name"].as<string>();
            }

            if (node["dst"].IsDefined())
            {
                for (std::size_t j = 0; j < node["dst"].size(); j++)
                {
                    ss.output_stream_settings.push_back(this->getOutputStreams(node["dst"][j]));
                }
            }
            result.push_back(ss);
        }
        return result;
    }
};

#endif // LIBAV_TEST_CONFIGSERVICE_H
