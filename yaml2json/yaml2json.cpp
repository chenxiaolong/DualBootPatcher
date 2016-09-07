#include <cerrno>
#include <cstring>

#include <getopt.h>

#include <jansson.h>
#include <yaml-cpp/yaml.h>


static void * xmalloc(size_t size)
{
    void *result = malloc(size);
    if (!result) {
        abort();
    }
    return result;
}

static json_t * yaml_node_to_json_node(const YAML::Node &yaml_node)
{
    switch (yaml_node.Type()) {
    case YAML::NodeType::Null:
        return json_object();

    case YAML::NodeType::Scalar:
        try {
            return json_integer(yaml_node.as<json_int_t>());
        } catch (const YAML::BadConversion &e) {}
        try {
            return json_real(yaml_node.as<double>());
        } catch (const YAML::BadConversion &e) {}
        try {
            return json_boolean(yaml_node.as<bool>());
        } catch (const YAML::BadConversion &e) {}
        try {
            return json_string(yaml_node.as<std::string>().c_str());
        } catch (const YAML::BadConversion &e) {}
        throw std::runtime_error("Cannot convert scalar value to known type");

    case YAML::NodeType::Sequence: {
        json_t *array = json_array();

        for (auto const &item : yaml_node) {
            json_array_append_new(array, yaml_node_to_json_node(item));
        }

        return array;
    }

    case YAML::NodeType::Map: {
        json_t *object = json_object();

        for (auto const &item : yaml_node) {
            json_object_set_new(object, item.first.as<std::string>().c_str(),
                                yaml_node_to_json_node(item.second));
        }

        return object;
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

    json_set_alloc_funcs(&xmalloc, &free);

    json_t *json_root = json_array();

    for (int i = optind; i < argc; ++i) {
        try {
            YAML::Node root = YAML::LoadFile(argv[i]);
            json_t *node = yaml_node_to_json_node(root);

            if (flatten_root_array && json_is_array(node)) {
                size_t index;
                json_t *elem;

                json_array_foreach(node, index, elem) {
                    json_array_append(json_root, elem);
                }

                json_decref(node);
            } else {
                json_array_append_new(json_root, node);
            }
        } catch (const std::exception &e) {
            fprintf(stderr, "%s: Failed to convert file: %s\n",
                    argv[i], e.what());
            return EXIT_FAILURE;
        }
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

    char *output;
    if (styled) {
        output = json_dumps(json_root, JSON_INDENT(4) | JSON_SORT_KEYS);
    } else {
        output = json_dumps(json_root, JSON_COMPACT);
    }

    if (fputs(output, fp) == EOF) {
        fprintf(stderr, "Failed to write JSON: %s\n", strerror(errno));
    }

    free(output);
    json_decref(json_root);

    if (output_file) {
        if (fclose(fp) != 0) {
            fprintf(stderr, "%s: Failed to close file: %s\n",
                    output_file, strerror(errno));
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}
