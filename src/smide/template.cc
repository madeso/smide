#include <iostream>
#include <string>
#include <map>
#include <fstream>
#include <optional>

#include "smide/rapidjson/document.h"
#include "smide/tinyxml2.h" // v11.0.0
#include "smide/mustache.hpp"

using namespace tinyxml2;
using namespace rapidjson;

enum
{
    MODE_ARG,
    INPUT_FILE,
    OUTPUT_FILE,
    ARG_COUNT
};


std::string file_to_error(const std::string& filename, XMLNode* node)
{
    std::ostringstream ss;
    ss << filename << '(';
    if (node)
    {
        ss << node->GetLineNum();
    }
    else
    {
        ss << "-1";
    }
    ss << "): ";
    return ss.str();
}

#define ERR(node, mess) std::cerr << file_to_error(filename, node)<< "error: " << mess << "\n"; status = false; continue

std::optional<std::string> load_mustache(const std::string& str, const kainjow::mustache::data& data)
{
    auto input = kainjow::mustache::mustache{ std::string{str.begin(), str.end()} };
    if (input.is_valid() == false)
    {
        const auto& error = input.error_message();
        std::cerr << "Failed to parse mustache: " << error << "\n";
        return std::nullopt;
    }

    input.set_custom_escape([](const std::string& s) { return s; });

    return input.render(data);
}

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

    const char* const mode_arg = argv[MODE_ARG];
    const char* const input_path = argv[INPUT_FILE];
    const char* const output_path = argv[OUTPUT_FILE];

    bool status = true;

    // parse file
    do
    {
        XMLDocument doc;
        const char* const filename = input_path;

        if (doc.LoadFile(filename) != XML_SUCCESS)
        {
            ERR(nullptr, "Failed to load file `" << filename << "`");
        }

        auto* root = doc.RootElement();
        if(root == nullptr)
        {
            ERR(nullptr, "Missing root");
        }

        auto* json_elem = root->FirstChildElement("json");
        if(json_elem != nullptr)
        {
            ERR(root, "Missing json elem");
        }

        const char* json_src = json_elem->GetText();
        if(json_src == nullptr)
        {
            ERR(json_elem, "Missing json text");
        }

        rapidjson::Document json;
        json.Parse<kParseCommentsFlag | kParseTrailingCommasFlag | kParseNanAndInfFlag>(json_src);
        const auto data = json_to_data(json);

        std::map<std::string, std::string> patterns;
        constexpr const char* pattern = "pattern";
        for(auto* elem = root->FirstChildElement(pattern); elem != nullptr; elem = elem->NextSiblingElement(pattern))
        {
            const char* name = elem->Attribute("name");
            if(name == nullptr)
            {
                ERR(elem, "missing name attribute");
            }

            const char* text = elem->GetText();
            if(text == nullptr)
            {
                ERR(elem, "elem is missing text");
            }
            patterns.insert({ name, text });
        }

        const auto requested_pattern = patterns.find(mode_arg);

        if(requested_pattern == patterns.end())
        {
            ERR(root, "Requested pattern not found: " << mode_arg);
        }

        const auto loaded = load_mustache(requested_pattern->second, data);
        if(loaded.has_value() == false)
        {
            ERR(nullptr, "failed to generate");
        }

        std::ofstream out(output_path);
        if(out.good() == false)
        {
            ERR(nullptr, "Failed to open file for writing: " << output_path);
        }
        out << *loaded;
    } while (false);

    return status ? 0 : -2;
}
