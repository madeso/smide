#include <iostream>
#include <string>
#include <fstream>
#include <sstream>

#include "smide/tinyxml2.h" // v11.0.0

using namespace tinyxml2;

enum
{
    OUTPUT_FILE,
    MODE_ARG,
    MACRO_ARG,
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

int main(int argc, char** argv)
{
    if(argc < ARG_COUNT)
    {
        std::cerr << "Invalid number of arguments\n";
        return -1;
    }

    bool status = true;

    const std::string mode_arg = argv[MODE_ARG];
    
    const char* const output_path = argv[OUTPUT_FILE];
    std::ofstream out(output_path);
    if (out.good() == false)
    {
        std::cerr << "Failed to open file for writing: " << output_path << "\n";
        return -2;
    }

    const std::string macro_arg = argv[MACRO_ARG];
    const bool add_line_directive = [macro_arg]()
        {
            if (macro_arg == "add_line") return true;
            if (macro_arg == "no_line") return false;

            std::cerr << "Invalid macro argument " << macro_arg << "\n";
            return false;
        }();

    // parse file
    for (int arg_index = ARG_COUNT; arg_index < argc; arg_index += 1)
    {
        const char* const filename = argv[arg_index];

        XMLDocument doc;

        if (doc.LoadFile(filename) != XML_SUCCESS)
        {
            ERR(nullptr, "Failed to load file `" << filename << "`");
        }

        auto* root = doc.RootElement();
        if(root == nullptr)
        {
            ERR(nullptr, "Missing root");
        }

        XMLElement* found = nullptr;
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

            if (mode_arg != name) continue;

            if(found != nullptr)
            {
                std::cerr << file_to_error(filename, elem) << "error: found duplicate node named " << name << "\n";
                std::cerr << file_to_error(filename, found) << "note: previous node found here" << "\n";
                status = false;
                continue;
            }

            found = elem;
            if(add_line_directive)
            {
                out << "#line " << elem->GetLineNum() << " \"" << filename << "\"\n";
            }

            out << text << "\n\n";
        }
    }

    return status ? 0 : -2;
}
