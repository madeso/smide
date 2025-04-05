#include "smide/tinyxml2.h" // v11.0.0
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>

using namespace tinyxml2;

enum
{
    APP_NAME_ARG,
    SOURCE_ARG,
    HEADER_ARG,
    ARG_COUNT
};

using Row = std::map<std::string, std::string>; // name -> value
using Table = std::vector<Row>;
using AllTables = std::map<std::string, Table>;

struct Output
{
    AllTables* tables;
    std::ofstream* source_file;
    std::ofstream* header_file;

    bool write_header = true;
    bool write_source = false;
    std::map<std::string, Row> variables;
    std::string between;

    Output(AllTables* t, std::ofstream* s, std::ofstream* h)
        : tables(t)
        , source_file(s)
        , header_file(h)
    {
    }

    void write_raw(const char* str) const
    {
        if(write_header)
        {
            (*header_file) << str;
        }
        if(write_source)
        {
            (*source_file) << str;
        }
    }

    void write_string(const std::string& str) const
    {
        write_raw(str.c_str());
    }

    [[nodiscard]] Output only_source() const
    {
        Output r = *this;
        r.write_header = false;
        r.write_source = true;
        return r;
    }

    [[nodiscard]] Output only_header() const
    {
        Output r = *this;
        r.write_header = true;
        r.write_source = false;
        return r;
    }

    [[nodiscard]] Output with_var(const std::string& name, const Row& row) const
    {
        Output r = *this;
        r.variables[name] = row;
        return r;
    }

    [[nodiscard]] Output with_between(const std::string& b) const
    {
        Output r = *this;
        r.between = b;
        return r;
    }

    // Output(const Output& rhs) = default;
    // Output(Output&& rhs) = default;
    // Output& operator=(const Output& rhs) = default;
    // Output& operator=(Output&& rhs) = default;
};

std::string file_to_error(const std::string& filename, XMLNode* node)
{
    std::ostringstream ss;
    ss << filename << '(';
    if(node)
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

std::string transform_string(const std::string& function, const std::string& value)
{
    if (function == "string")
    {
        std::ostringstream ss;
        ss << "\"";
        for(char c: value)
        {
            switch(c)
            {
            case '\\':
            case '\'':
            case '"':
                ss << '\\';
                ss << c;
                break;
            case '\n':
                ss << "\\n";
                break;
            case '\t':
                ss << "\\t";
                break;
            default:
                ss << c;
                break;
            }
        }
        ss << "\"";
        return ss.str();
    }
    else if(function == "raw" || function == "none")
    {
        return value;
    }
    else
    {
        std::cerr << "Unknown function " << function << "\n";
        return value;
    }
}

bool generate_rows(const std::string& filename, XMLElement* root, const Output& o)
{
    const auto expand = [&filename](const Output& o, const Table& table, const std::string& var_name, XMLElement* elem)
    {
        bool status = true;
        bool first = true;
        for (const auto& row : table)
        {
            if (first) first = false;
            else o.write_string(o.between);

            status = generate_rows(filename, elem, o.with_var(var_name, row)) && status;
        }
        return status;
    };
    bool status = true;
    for(auto* child = root->FirstChild(); child; child = child->NextSibling())
    {
        auto* text = child->ToText();
        if(text != nullptr)
        {
            o.write_raw(text->Value());
            continue;
        }

        auto* elem = child->ToElement();
        if(elem)
        {
            const std::string name = elem->Name();
            if(name == "source")
            {
                status = generate_rows(filename, elem, o.only_source()) && status;
            }
            else if (name == "header")
            {
                status = generate_rows(filename, elem, o.only_header()) && status;
            }
            else if (name == "expand")
            {
                const char* table = elem->Attribute("table");
                if(table == nullptr)
                {
                    ERR(elem, "Failed to find table prop");
                }
                const auto& found = o.tables->find(table);
                if(found == o.tables->end())
                {
                    ERR(elem, "Failed to find table " << table);
                }

                const char* var_name = elem->Attribute("var");
                if(var_name == nullptr)
                {
                    ERR(elem, "Failed to find prop var");
                }

                status = expand(o,  found->second, var_name, elem) && status;
            }
            else if(name == "var")
            {
                const char* table = elem->Attribute("name");
                if(table == nullptr)
                {
                    ERR(elem, "Missing name property in var");
                }

                const auto found_row = o.variables.find(table);
                if(found_row == o.variables.end())
                {
                    ERR(elem, table << " is not a expanded table");
                }
                const auto& row = found_row->second;

                const char* col = elem->Attribute("col");
                if(col == nullptr)
                {
                    ERR(elem, "Missing col property in var");
                }
                const auto found_var = row.find(col);
                if(found_var == row.end())
                {
                    // todo(Gustav): add a name to the table and not just the variable
                    ERR(elem, col << " is not a column in " << table);
                }

                const char* transform = elem->Attribute("transform");
                o.write_string(transform ? transform_string(transform, found_var->second) : found_var->second);
            }
            else if(name == "enum")
            {
                const char* name = elem->Attribute("name");
                if(name == nullptr)
                {
                    ERR(elem, "Missing name property in enum");
                }
                const auto& h = o.only_header();
                h.write_raw("enum class ");
                h.write_raw(name);
                h.write_raw("{");
                status = generate_rows(filename, elem, h) && status;
                h.write_raw("\n};\n");

            }
            else if(name == "expand_data")
            {
                const char* name = elem->Attribute("name");
                if (name == nullptr)
                {
                    ERR(elem, "Missing name property in data");
                }
                const char* table = elem->Attribute("table");
                if (table == nullptr)
                {
                    ERR(elem, "Failed to find table prop");
                }
                const auto& found = o.tables->find(table);
                if (found == o.tables->end())
                {
                    ERR(elem, "Failed to find table " << table);
                }

                const char* var_name = elem->Attribute("var");
                if (var_name == nullptr)
                {
                    ERR(elem, "Failed to find prop var");
                }

                const auto num_entries = found->second.size();
                std::ostringstream out_entries;
                out_entries << '[' << num_entries << ']';

                const auto& h = o.only_header();
                h.write_raw("extern ");
                h.write_raw(name);
                h.write_string(out_entries.str());

                h.write_raw(";\n");

                const auto& s = o.only_source();
                s.write_raw(name);
                s.write_string(out_entries.str());
                s.write_raw(" = {\n");

                status = expand(s.with_between(", "), found->second, var_name, elem) && status;

                s.write_raw("\n};\n");
            }
            else
            {
                ERR(elem, "Invalid element " << name);
            }
        }
    }

    return status;
}

int main(int argc, char** argv)
{
    if(argc < ARG_COUNT)
    {
        std::cerr << "Invalid number of arguments\n";
        return -1;
    }

    const char* const source_name = argv[SOURCE_ARG];
    const char* const header_name = argv[HEADER_ARG];

    std::ofstream source_file{source_name};
    // todo(Gustav): generate include directive

    std::ofstream header_file{header_name};
    header_file << "#pragma once\n\n";

    bool status = true;

    for(int arg_index = ARG_COUNT; arg_index < argc; arg_index += 1)
    {
        XMLDocument doc;
        const char* const filename = argv[arg_index];

        if(doc.LoadFile(filename) != XML_SUCCESS)
        {
            ERR(nullptr, "Failed to load file `" << filename << "`");
        }

        auto* root = doc.RootElement();
        if(root == nullptr)
        {
            ERR(nullptr, "Missing root element");
        }

        auto* tables_list_elem = root->FirstChildElement("tables");
        auto* gen = root->FirstChildElement("gen");

        if(tables_list_elem == nullptr)
        {
            ERR(root, "Missing tables element");
        }
        if (gen == nullptr)
        {
            ERR(root, "Missing gen element");
        }

        // load tables
        AllTables all_tables;
        for(auto* table_elem = tables_list_elem->FirstChildElement(); table_elem; table_elem = table_elem->NextSiblingElement())
        {
            Row default_values;
            constexpr const char* COL_NAME = "col";
            for (auto* col_elem = table_elem->FirstChildElement(COL_NAME); col_elem; col_elem = col_elem->NextSiblingElement(COL_NAME))
            {
                const char* const col_name = col_elem->Attribute("name");
                const char* const col_def = col_elem->Attribute("default");
                if(col_name == nullptr)
                {
                    ERR(col_elem, "Missing name");
                }
                default_values.insert(Row::value_type(col_name, col_def ? col_def : ""));
            }
            Table tab;
            constexpr const char* ROW_NAME = "row";
            for (auto* row_elem = table_elem->FirstChildElement(ROW_NAME); row_elem; row_elem = row_elem->NextSiblingElement(ROW_NAME))
            {
                Row row;
                for(const auto& [arg_name, default_val] : default_values)
                {
                    const char* const value = row_elem->Attribute(arg_name.c_str());
                    row.insert(Row::value_type(arg_name, value ? value : default_val));
                }
                tab.push_back(row);
            }
            all_tables.insert(AllTables::value_type(table_elem->Name(), tab));
        }

        status = generate_rows(filename, gen, Output{ &all_tables, &source_file, &header_file }) && status;
    }

    return status ? 0 : -2;
}
