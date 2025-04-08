#include <iostream>
#include <string>
#include <map>
#include <fstream>
#include <optional>

#include "smide/rapidjson/document.h"
#include "smide/mustache.hpp"

using namespace rapidjson;

enum
{
    APP_NAME_ARG,
    MODE_ARG,
    INPUT_FILE,
    OUTPUT_FILE,
    ARG_COUNT
};

#define ERR(mess) std::cerr << "error: " << mess << "\n"; status = false; continue


kainjow::mustache::data json_to_data(const rapidjson::Value& doc)
{
    kainjow::mustache::data ret;

    if (!doc.IsObject())
    {
        std::cerr << "JSON document is not an object\n";
        return ret;
    }

    for (auto it = doc.MemberBegin(); it != doc.MemberEnd(); ++it)
    {
        const std::string key = it->name.GetString();

        if (it->value.IsObject())
        {
            ret[key] = json_to_data(it->value.GetObject());
        }
        else if (it->value.IsArray())
        {
            kainjow::mustache::list list_data;
            for (auto& array_elem : it->value.GetArray())
            {
                if (array_elem.IsObject())
                {
                    list_data.push_back(json_to_data(array_elem.GetObject()));
                }
                else if (array_elem.IsString())
                {
                    list_data.push_back(array_elem.GetString());
                }
                else
                {
                    std::cerr << "Unsupported array element type for key: " << key << "\n";
                }
            }
            ret[key] = list_data;
        }
        else if (it->value.IsString())
        {
            ret[key] = it->value.GetString();
        }
        else if (it->value.IsBool())
        {
            ret[key] = it->value.GetBool() ? "true" : "false";
        }
        else if (it->value.IsNumber())
        {
            ret[key] = std::to_string(it->value.GetDouble());
        }
        else
        {
            std::cerr << "Unsupported JSON value type for key: " << key << "\n";
        }
    }

    return ret;
}


int main(int argc, char** argv)
{
    if(argc != ARG_COUNT)
    {
        std::cerr << "Invalid number of arguments\n";
        return -1;
    }

    const char* const pattern_path = argv[MODE_ARG];
    const char* const input_path = argv[INPUT_FILE];
    const char* const output_path = argv[OUTPUT_FILE];

    // ================================================================
    // load json input
    std::string json_src;
    {
        std::ifstream f(input_path);
        if(f.good() == false)
        {
            std::cerr << "Failed to open " << input_path << "\n";
            return -1;
        }
        json_src.assign((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    }
    rapidjson::Document json;
    json.Parse<kParseCommentsFlag | kParseTrailingCommasFlag | kParseNanAndInfFlag>(json_src.c_str());
    const auto data = json_to_data(json);


    // ================================================================
    // load pattern
    std::string pattern_src;
    {
        std::ifstream f(pattern_path);
        if (f.good() == false)
        {
            std::cerr << "Failed to open " << pattern_path << "\n";
            return -1;
        }
        pattern_src.assign((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    }
    auto input = kainjow::mustache::mustache{ pattern_src };
    if (input.is_valid() == false)
    {
        const auto& error = input.error_message();
        std::cerr << "Failed to parse mustache: " << error << "\n";
        return - 1;
    }
    input.set_custom_escape([](const std::string& s) { return s; });


    // ================================================================
    // write output file
    std::ofstream out(output_path);
    if (out.good() == false)
    {
        std::cerr << "Failed to open file for writing: " << output_path;
        return -1;
    }
    out << input.render(data);

    return 0;
}
