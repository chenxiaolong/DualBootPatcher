/*
 * Copyright (C) 2014-2015  Andrew Gunnerson <andrewgunnerson@gmail.com>
 *
 * This file is part of DualBootPatcher
 *
 * DualBootPatcher is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * DualBootPatcher is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with DualBootPatcher.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "mbpatcher/autopatchers/standardpatcher.h"

#include <cstring>

#include "mbcommon/string.h"
#include "mblog/logging.h"

#include "mbpatcher/edify/tokenizer.h"
#include "mbpatcher/private/fileutils.h"
#include "mbpatcher/private/stringutils.h"

#define LOG_TAG "mbpatcher/autopatchers/standardpatcher"

#define DUMP_DEBUG 0


namespace mb
{
namespace patcher
{

/*! \cond INTERNAL */
class StandardPatcherPrivate
{
public:
    const PatcherConfig *pc;
    const FileInfo *info;
};
/*! \endcond */


const std::string StandardPatcher::Id
        = "StandardPatcher";

const std::string StandardPatcher::UpdaterScript
        = "META-INF/com/google/android/updater-script";

const std::string StandardPatcher::SystemTransferList
        = "system.transfer.list";

static constexpr char MOUNT_FMT[] =
        "(run_program(\"/update-binary-tool\", \"mount\", \"%s\") == 0)";
static constexpr char UNMOUNT_FMT[] =
        "(run_program(\"/update-binary-tool\", \"unmount\", \"%s\") == 0)";
static constexpr char FORMAT_FMT[] =
        "(run_program(\"/update-binary-tool\", \"format\", \"%s\") == 0)";


StandardPatcher::StandardPatcher(const PatcherConfig * const pc,
                                 const FileInfo * const info)
    : _priv_ptr(new StandardPatcherPrivate())
{
    MB_PRIVATE(StandardPatcher);
    priv->pc = pc;
    priv->info = info;
}

StandardPatcher::~StandardPatcher()
{
}

ErrorCode StandardPatcher::error() const
{
    return ErrorCode();
}

std::string StandardPatcher::id() const
{
    return Id;
}

std::vector<std::string> StandardPatcher::new_files() const
{
    return {};
}

std::vector<std::string> StandardPatcher::existing_files() const
{
    return { UpdaterScript, SystemTransferList };
}

static bool find_items_in_string(const std::string &haystack,
                                 const std::vector<std::string> &needles)
{
    for (auto const &needle : needles) {
        if (haystack.find(needle) != std::string::npos) {
            return true;
        }
    }

    return false;
}

static bool find_function(const std::vector<EdifyToken *>::iterator begin,
                          const std::vector<EdifyToken *>::iterator end,
                          std::vector<EdifyToken *>::iterator *out_func_name,
                          std::vector<EdifyToken *>::iterator *out_left_paren,
                          std::vector<EdifyToken *>::iterator *out_right_paren)
{
    std::vector<EdifyToken *>::iterator func_name;
    std::vector<EdifyToken *>::iterator left_paren;
    std::vector<EdifyToken *>::iterator right_paren;

    for (auto it = begin; it != end; ++it) {
        // Find string representing the function name
        if ((*it)->type() != EdifyTokenType::String) {
            continue;
        }

        func_name = it;

        bool found_left_paren = false;
        bool found_right_paren = false;

        // Barring any whitespace, newlines, or comments, the function name
        // should be followed by a left parenthesis
        for (auto it2 = it + 1; it2 != end; ++it2) {
            if ((*it2)->type() == EdifyTokenType::Whitespace
                    || (*it2)->type() == EdifyTokenType::Newline
                    || (*it2)->type() == EdifyTokenType::Comment) {
                continue;
            } else if ((*it2)->type() == EdifyTokenType::LeftParen) {
                found_left_paren = true;
                left_paren = it2;
            }
            break;
        }

        // If a left parenthesis was not found, then the string token was not
        // a function name
        if (!found_left_paren) {
            continue;
        }

        // Left for matching right parenthesis
        std::size_t depth = 0;

        for (auto it2 = left_paren; it2 != end; ++it2) {
            if ((*it2)->type() == EdifyTokenType::LeftParen) {
                ++depth;
            } else if ((*it2)->type() == EdifyTokenType::RightParen) {
                --depth;
            }
            if (depth == 0) {
                found_right_paren = true;
                right_paren = it2;
                break;
            }
        }

        // If a right parenthesis was not found, but the function name and left
        // parenthesis were found, then assume there's a syntax error and bail
        // out
        if (!found_right_paren) {
            return false;
        }

        *out_func_name = func_name;
        *out_left_paren = left_paren;
        *out_right_paren = right_paren;

        return true;
    }

    return false;
}

/*!
 * \brief Replace edify function
 *
 * \param tokens List of edify tokens
 * \param func_name Function name token of the replaced function
 * \param left_paren Left parenthesis token of the replaced function
 * \param right_paren Right parenthesis token of the replaced function
 * \param replacement Replacement edify function (in string form)
 *
 * \return New iterator pointing to position *after* the right parenthesis of
 *         the replaced function. Returns tokens->end() if the replacement
 *         string could not be tokenized.
 */
static std::vector<EdifyToken *>::iterator
replace_function(std::vector<EdifyToken *> *tokens,
                 std::vector<EdifyToken *>::iterator func_name,
                 std::vector<EdifyToken *>::iterator left_paren,
                 std::vector<EdifyToken *>::iterator right_paren,
                 const std::string &replacement)
{
    // Included for completeness' sake
    (void) left_paren;

    std::vector<EdifyToken *> replacement_tokens;
    bool result = EdifyTokenizer::tokenize(
            replacement.data(), replacement.size(), &replacement_tokens);
    if (!result) {
        LOGE("Failed to tokenize replacement function string: %s",
             replacement.c_str());
        return tokens->end();
    }

    // Deallocate replaced tokens
    for (auto it = func_name; it != right_paren; ++it) {
        delete *it;
    }
    delete *right_paren;

    // Remove replaced tokens
    auto it = tokens->erase(func_name, right_paren);
    it = tokens->erase(it);

    // Add replacement tokens
    it = tokens->insert(it, replacement_tokens.begin(),
                        replacement_tokens.end());

    // Move iterator to the end of the replaced tokens
    it += static_cast<ptrdiff_t>(replacement_tokens.size());

    return it;
}

/*!
 * \brief Replace edify mount() command
 *
 * \param tokens List of edify tokens
 * \param func_name Function name token
 * \param left_paren Left parenthesis token
 * \param right_paren Right parenthesis token
 * \param system_devs List of system partition block devices
 * \param cache_devs List of cache partition block devices
 * \param data_devs List of data partition block devices
 *
 * \return Iterator pointing to position immediately after the right parenthesis
 */
static std::vector<EdifyToken *>::iterator
replace_edify_mount(std::vector<EdifyToken *> *tokens,
                    const std::vector<EdifyToken *>::iterator func_name,
                    const std::vector<EdifyToken *>::iterator left_paren,
                    const std::vector<EdifyToken *>::iterator right_paren,
                    const std::vector<std::string> &system_devs,
                    const std::vector<std::string> &cache_devs,
                    const std::vector<std::string> &data_devs)
{
    // For the mount() edify function, replace with the corresponding
    // update-binary-tool command
    for (auto it = left_paren + 1; it != right_paren; ++it) {
        if ((*it)->type() != EdifyTokenType::String) {
            continue;
        }

        auto token = static_cast<EdifyTokenString *>(*it);
        const std::string str = token->string();

        bool is_system = str.find("/system") != std::string::npos
                || find_items_in_string(str.c_str(), system_devs);
        bool is_cache = str.find("/cache") != std::string::npos
                || find_items_in_string(str.c_str(), cache_devs);
        bool is_data = str.find("/data") != std::string::npos
                || str.find("/userdata") != std::string::npos
                || find_items_in_string(str.c_str(), data_devs);

        if (is_system) {
            return replace_function(tokens, func_name, left_paren, right_paren,
                                    format(MOUNT_FMT, "/system"));
        } else if (is_cache) {
            return replace_function(tokens, func_name, left_paren, right_paren,
                                    format(MOUNT_FMT, "/cache"));
        } else if (is_data) {
            return replace_function(tokens, func_name, left_paren, right_paren,
                                    format(MOUNT_FMT, "/data"));
        }
    }
    return right_paren + 1;
}

/*!
 * \brief Replace edify unmount() command
 *
 * \param tokens List of edify tokens
 * \param func_name Function name token
 * \param left_paren Left parenthesis token
 * \param right_paren Right parenthesis token
 * \param system_devs List of system partition block devices
 * \param cache_devs List of cache partition block devices
 * \param data_devs List of data partition block devices
 *
 * \return Iterator pointing to position immediately after the right parenthesis
 */
static std::vector<EdifyToken *>::iterator
replace_edify_unmount(std::vector<EdifyToken *> *tokens,
                      const std::vector<EdifyToken *>::iterator func_name,
                      const std::vector<EdifyToken *>::iterator left_paren,
                      const std::vector<EdifyToken *>::iterator right_paren,
                      const std::vector<std::string> &system_devs,
                      const std::vector<std::string> &cache_devs,
                      const std::vector<std::string> &data_devs)
{
    // For the unmount() edify function, replace with the corresponding
    // update-binary-tool command
    for (auto it = left_paren + 1; it != right_paren; ++it) {
        if ((*it)->type() != EdifyTokenType::String) {
            continue;
        }

        auto token = static_cast<EdifyTokenString *>(*it);
        const std::string str = token->string();

        bool is_system = str.find("/system") != std::string::npos
                || find_items_in_string(str.c_str(), system_devs);
        bool is_cache = str.find("/cache") != std::string::npos
                || find_items_in_string(str.c_str(), cache_devs);
        bool is_data = str.find("/data") != std::string::npos
                || str.find("/userdata") != std::string::npos
                || find_items_in_string(str.c_str(), data_devs);

        if (is_system) {
            return replace_function(tokens, func_name, left_paren, right_paren,
                                    format(UNMOUNT_FMT, "/system"));
        } else if (is_cache) {
            return replace_function(tokens, func_name, left_paren, right_paren,
                                    format(UNMOUNT_FMT, "/cache"));
        } else if (is_data) {
            return replace_function(tokens, func_name, left_paren, right_paren,
                                    format(UNMOUNT_FMT, "/data"));
        }
    }
    return right_paren + 1;
}

/*!
 * \brief Replace edify run_program() command
 *
 * \param tokens List of edify tokens
 * \param func_name Function name token
 * \param left_paren Left parenthesis token
 * \param right_paren Right parenthesis token
 * \param system_devs List of system partition block devices
 * \param cache_devs List of cache partition block devices
 * \param data_devs List of data partition block devices
 *
 * \return Iterator pointing to position immediately after the right parenthesis
 */
static std::vector<EdifyToken *>::iterator
replace_edify_run_program(std::vector<EdifyToken *> *tokens,
                          const std::vector<EdifyToken *>::iterator func_name,
                          const std::vector<EdifyToken *>::iterator left_paren,
                          const std::vector<EdifyToken *>::iterator right_paren,
                          const std::vector<std::string> &system_devs,
                          const std::vector<std::string> &cache_devs,
                          const std::vector<std::string> &data_devs)
{
    bool found_reboot = false;
    bool found_mount = false;
    bool found_umount = false;
    bool found_format_sh = false;
    bool found_mke2fs = false;
    bool is_system = false;
    bool is_cache = false;
    bool is_data = false;

    for (auto it = left_paren + 1; it != right_paren; ++it) {
        if ((*it)->type() != EdifyTokenType::String) {
            continue;
        }

        auto token = static_cast<EdifyTokenString *>(*it);
        const std::string unescaped = token->unescaped_string();

        if (ends_with(unescaped, "reboot")) {
            found_reboot = true;
        }
        if (ends_with(unescaped, "mount")) {
            found_mount = true;
        }
        if (ends_with(unescaped, "umount")) {
            found_umount = true;
        }
        if (ends_with(unescaped, "/format.sh")) {
            found_format_sh = true;
        }
        if (ends_with(unescaped, "/mke2fs")) {
            found_mke2fs = true;
        }

        if (unescaped.find("/system") != std::string::npos
                || find_items_in_string(unescaped.c_str(), system_devs)) {
            is_system = true;
        }
        if (unescaped.find("/cache") != std::string::npos
                || find_items_in_string(unescaped.c_str(), cache_devs)) {
            is_cache = true;
        }
        if (unescaped.find("/data") != std::string::npos
                || unescaped.find("/userdata") != std::string::npos
                || find_items_in_string(unescaped.c_str(), data_devs)) {
            is_data = true;
        }
    }

    if (found_reboot) {
        return replace_function(tokens, func_name, left_paren, right_paren,
                                "(ui_print(\"Removed reboot command\") == 0)");
    } else if (found_umount) {
        if (is_system) {
            return replace_function(tokens, func_name, left_paren, right_paren,
                                    format(UNMOUNT_FMT, "/system"));
        } else if (is_cache) {
            return replace_function(tokens, func_name, left_paren, right_paren,
                                    format(UNMOUNT_FMT, "/cache"));
        } else if (is_data) {
            return replace_function(tokens, func_name, left_paren, right_paren,
                                    format(UNMOUNT_FMT, "/data"));
        }
    } else if (found_mount) {
        if (is_system) {
            return replace_function(tokens, func_name, left_paren, right_paren,
                                    format(MOUNT_FMT, "/system"));
        } else if (is_cache) {
            return replace_function(tokens, func_name, left_paren, right_paren,
                                    format(MOUNT_FMT, "/cache"));
        } else if (is_data) {
            return replace_function(tokens, func_name, left_paren, right_paren,
                                    format(MOUNT_FMT, "/data"));
        }
    } else if (found_format_sh) {
        return replace_function(tokens, func_name, left_paren, right_paren,
                                format(FORMAT_FMT, "/system"));
    } else if (found_mke2fs) {
        if (is_system) {
            return replace_function(tokens, func_name, left_paren, right_paren,
                                    format(FORMAT_FMT, "/system"));
        } else if (is_cache) {
            return replace_function(tokens, func_name, left_paren, right_paren,
                                    format(FORMAT_FMT, "/cache"));
        } else if (is_data) {
            return replace_function(tokens, func_name, left_paren, right_paren,
                                    format(FORMAT_FMT, "/data"));
        }
    }

    return right_paren + 1;
}

/*!
 * \brief Replace edify delete_recursive() command
 *
 * \param tokens List of edify tokens
 * \param func_name Function name token
 * \param left_paren Left parenthesis token
 * \param right_paren Right parenthesis token
 *
 * \return Iterator pointing to position immediately after the right parenthesis
 */
static std::vector<EdifyToken *>::iterator
replace_edify_delete_recursive(std::vector<EdifyToken *> *tokens,
                               const std::vector<EdifyToken *>::iterator func_name,
                               const std::vector<EdifyToken *>::iterator left_paren,
                               const std::vector<EdifyToken *>::iterator right_paren)
{
    for (auto it = left_paren + 1; it != right_paren; ++it) {
        if ((*it)->type() != EdifyTokenType::String) {
            continue;
        }

        auto token = static_cast<EdifyTokenString *>(*it);
        const std::string unescaped = token->unescaped_string();

        if (unescaped == "/system" || unescaped == "/system/") {
            return replace_function(tokens, func_name, left_paren, right_paren,
                                    format(FORMAT_FMT, "/system"));
        } else if (unescaped == "/cache" || unescaped == "/cache/") {
            return replace_function(tokens, func_name, left_paren, right_paren,
                                    format(FORMAT_FMT, "/cache"));
        }
    }
    return right_paren + 1;
}

/*!
 * \brief Replace edify format() command
 *
 * \param tokens List of edify tokens
 * \param func_name Function name token
 * \param left_paren Left parenthesis token
 * \param right_paren Right parenthesis token
 * \param system_devs List of system partition block devices
 * \param cache_devs List of cache partition block devices
 * \param data_devs List of data partition block devices
 *
 * \return Iterator pointing to position immediately after the right parenthesis
 */
static std::vector<EdifyToken *>::iterator
replace_edify_format(std::vector<EdifyToken *> *tokens,
                     const std::vector<EdifyToken *>::iterator func_name,
                     const std::vector<EdifyToken *>::iterator left_paren,
                     const std::vector<EdifyToken *>::iterator right_paren,
                     const std::vector<std::string> &system_devs,
                     const std::vector<std::string> &cache_devs,
                     const std::vector<std::string> &data_devs)
{
    // For the format() edify function, replace with the corresponding
    // update-binary-tool command
    for (auto it = left_paren + 1; it != right_paren; ++it) {
        if ((*it)->type() != EdifyTokenType::String) {
            continue;
        }

        auto token = static_cast<EdifyTokenString *>(*it);
        const std::string str = token->string();

        bool is_system = str.find("/system") != std::string::npos
                || find_items_in_string(str.c_str(), system_devs);
        bool is_cache = str.find("/cache") != std::string::npos
                || find_items_in_string(str.c_str(), cache_devs);
        bool is_data = str.find("/data") != std::string::npos
                || str.find("/userdata") != std::string::npos
                || find_items_in_string(str.c_str(), data_devs);

        if (is_system) {
            return replace_function(tokens, func_name, left_paren, right_paren,
                                    format(FORMAT_FMT, "/system"));
        } else if (is_cache) {
            return replace_function(tokens, func_name, left_paren, right_paren,
                                    format(FORMAT_FMT, "/cache"));
        } else if (is_data) {
            return replace_function(tokens, func_name, left_paren, right_paren,
                                    format(FORMAT_FMT, "/data"));
        }
    }
    return right_paren + 1;
}

bool StandardPatcher::patch_files(const std::string &directory)
{
    if (!patch_updater(directory)) {
        return false;
    }

    if (!patch_transfer_list(directory)) {
        return false;
    }

    return true;
}

bool StandardPatcher::patch_updater(const std::string &directory)
{
    MB_PRIVATE(StandardPatcher);

    std::string contents;
    std::string path;

    path += directory;
    path += "/";
    path += UpdaterScript;

    FileUtils::read_to_string(path, &contents);

    if (contents.size() >= 2 && std::memcmp(contents.data(), "#!", 2) == 0) {
        // Ignore any script with a shebang line
        return true;
    }

    std::vector<EdifyToken *> tokens;
    bool result = EdifyTokenizer::tokenize(
            contents.data(), contents.size(), &tokens);
    if (!result) {
        LOGE("Failed to tokenize updater-script");
        return false;
    }

#if DUMP_DEBUG
    EdifyTokenizer::dump(tokens);
#endif

    auto &&device = priv->info->device();
    auto system_devs = device.system_block_devs();
    auto cache_devs = device.cache_block_devs();
    auto data_devs = device.data_block_devs();

    std::vector<EdifyToken *>::iterator begin = tokens.begin();
    std::vector<EdifyToken *>::iterator end;

    // TODO: Catch errors
    while (true) {
        end = tokens.end();

        // Need to find:
        // 1. String containing function name
        // 2. Left parenthesis for the function
        // 3. Right parenthesis for the function
        std::vector<EdifyToken *>::iterator func_name;
        std::vector<EdifyToken *>::iterator left_paren;
        std::vector<EdifyToken *>::iterator right_paren;

        if (!find_function(begin, end, &func_name, &left_paren, &right_paren)) {
            break;
        }

        // Tokens (types are checked by findFunction())
        auto t_func_name = static_cast<EdifyTokenString *>(*func_name);

        if (t_func_name->unescaped_string() == "mount") {
            begin = replace_edify_mount(&tokens, func_name, left_paren, right_paren,
                                        system_devs, cache_devs, data_devs);
        } else if (t_func_name->unescaped_string() == "unmount") {
            begin = replace_edify_unmount(&tokens, func_name, left_paren, right_paren,
                                          system_devs, cache_devs, data_devs);
        } else if (t_func_name->unescaped_string() == "run_program") {
            begin = replace_edify_run_program(&tokens, func_name, left_paren, right_paren,
                                              system_devs, cache_devs, data_devs);
        } else if (t_func_name->unescaped_string() == "delete_recursive") {
            begin = replace_edify_delete_recursive(&tokens, func_name, left_paren, right_paren);
        } else if (t_func_name->unescaped_string() == "format") {
            begin = replace_edify_format(&tokens, func_name, left_paren, right_paren,
                                         system_devs, cache_devs, data_devs);
        } else {
            begin = func_name + 1;
        }
    }

#if DUMP_DEBUG
    EdifyTokenizer::dump(tokens);
#endif

    FileUtils::write_from_string(path, EdifyTokenizer::untokenize(tokens));

    for (EdifyToken *t : tokens) {
        delete t;
    }

    return true;
}

bool StandardPatcher::patch_transfer_list(const std::string &directory)
{
    std::string contents;
    std::string path;
    std::vector<std::string> lines;

    path += directory;
    path += "/";
    path += SystemTransferList;

    auto ret = FileUtils::read_to_string(path, &contents);
    if (ret != ErrorCode::NoError) {
        return ret == ErrorCode::FileOpenError;
    }

    lines = StringUtils::split(contents, '\n');

    for (auto it = lines.begin(); it != lines.end();) {
        if (starts_with(*it, "erase ")) {
            it = lines.erase(it);
        } else {
            ++it;
        }
    }

    contents = StringUtils::join(lines, "\n");
    FileUtils::write_from_string(path, contents);

    return true;
}

}
}
