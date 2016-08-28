#include <cerrno>
#include <cstring>

#include <getopt.h>

#include <json/json.h>
#include <yaml-cpp/yaml.h>

#define TRY_RETURN(NODE, AS) \
    try { \
        return Json::Value((NODE).as<AS>()); \
    } catch (const YAML::BadConversion &e) { \
    }

static Json::Value yaml_node_to_json_node(const YAML::Node &yaml_node)
{
    switch (yaml_node.Type()) {
    case YAML::NodeType::Null:
        return Json::Value();

    case YAML::NodeType::Scalar:
        TRY_RETURN(yaml_node, Json::UInt64);
        TRY_RETURN(yaml_node, Json::Int64);
        TRY_RETURN(yaml_node, double);
        TRY_RETURN(yaml_node, bool);
        TRY_RETURN(yaml_node, std::string);
        throw std::runtime_error("Cannot convert scalar value to known type");

    case YAML::NodeType::Sequence: {
        Json::Value value(Json::arrayValue);

        for (auto const &item : yaml_node) {
            value.append(yaml_node_to_json_node(item));
        }

        return value;
    }

    case YAML::NodeType::Map: {
        Json::Value value(Json::objectValue);

        for (auto const &item : yaml_node) {
            value[item.first.as<std::string>()] =
                    yaml_node_to_json_node(item.second);
        }

        return value;
    }

    case YAML::NodeType::Undefined:
    default:
        throw std::runtime_error("Unknown YAML node type");
    }
}

static void usage(FILE *stream)
{
    fprintf(stream,
            "Usage: yaml2json [OPTION]...\n"
            "\n"
            "Options:\n"
            "  -o, --output <file>\n"
            "                   Output file (outputs to stdout if omitted)\n"
            "  -h, --help       Display this help message\n"
            "  --flatten-root-array\n"
            "                   Flatten if root is array\n"
            "  --styled         Output in human-readable format\n");
}

int main(int argc, char *argv[])
{
    int opt;

    enum Options {
        OPT_FLATTEN_ROOT_ARRAY = 1000,
        OPT_STYLED             = 1001,
    };

    static const char short_options[] = "o:h";

    static struct option long_options[] = {
        {"flatten-root-array", no_argument, 0, OPT_FLATTEN_ROOT_ARRAY},
        {"styled", no_argument, 0, OPT_STYLED},
        {"output", required_argument, 0, 'o'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    int long_index = 0;

    const char *output_file = nullptr;
    bool flatten_root_array = false;
    bool styled = false;

    while ((opt = getopt_long(argc, argv, short_options,
                              long_options, &long_index)) != -1) {
        switch (opt) {
        case OPT_FLATTEN_ROOT_ARRAY:
            flatten_root_array = true;
            break;

        case OPT_STYLED:
            styled = true;
            break;

        case 'o':
            output_file = optarg;
            break;

        case 'h':
            usage(stdout);
            return EXIT_SUCCESS;

        default:
            usage(stderr);
            return EXIT_FAILURE;
        }
    }

    Json::Value json_root(Json::arrayValue);

    for (int i = optind; i < argc; ++i) {
        try {
            YAML::Node root = YAML::LoadFile(argv[i]);
            Json::Value node = yaml_node_to_json_node(root);

            if (flatten_root_array && node.type() == Json::arrayValue) {
                for (auto const &elem : node) {
                    json_root.append(elem);
                }
            } else {
                json_root.append(node);
            }
        } catch (const std::exception &e) {
            fprintf(stderr, "%s: Failed to convert file: %s\n",
                    argv[i], e.what());
            return EXIT_FAILURE;
        }
    }

    std::string output;
    if (styled) {
        output = Json::StyledWriter().write(json_root);
    } else {
        output = Json::FastWriter().write(json_root);
    }

    FILE *fp = stdout;

    if (output_file) {
        fp = fopen(output_file, "w");
        if (!fp) {
            fprintf(stderr, "%s: Failed to open file: %s\n",
                    output_file, strerror(errno));
            return EXIT_FAILURE;
        }
    }

    if (fputs(output.c_str(), fp) == EOF) {
        fprintf(stderr, "Failed to write JSON: %s\n", strerror(errno));
    }

    if (output_file) {
        if (fclose(fp) != 0) {
            fprintf(stderr, "%s: Failed to close file: %s\n",
                    output_file, strerror(errno));
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}
