/*
 * Copyright (C) 2014-2017  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include <optional>

#include <cstring>

#include "mbcommon/string.h"
#include "mblog/logging.h"

#include "mbpatcher/edify/tokenizer.h"
#include "mbpatcher/private/fileutils.h"

#define LOG_TAG "mbpatcher/autopatchers/standardpatcher"

#define DUMP_DEBUG 0


namespace mb::patcher
{

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


StandardPatcher::StandardPatcher(const PatcherConfig &pc, const FileInfo &info)
    : m_info(info)
{
    (void) pc;
}

StandardPatcher::~StandardPatcher() = default;

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

using TokenIter = std::vector<EdifyToken>::iterator;

struct FunctionBounds
{
    TokenIter func_name;
    TokenIter left_paren;
    TokenIter right_paren;
};

static std::optional<FunctionBounds>
find_function(const TokenIter begin, const TokenIter end)
{
    FunctionBounds bounds;

    for (auto it = begin; it != end; ++it) {
        // Find string representing the function name
        if (!std::holds_alternative<EdifyTokenString>(*it)) {
            continue;
        }

        bounds.func_name = it;

        bool found_left_paren = false;
        bool found_right_paren = false;

        // Barring any whitespace, newlines, or comments, the function name
        // should be followed by a left parenthesis
        for (auto it2 = it + 1; it2 != end; ++it2) {
            if (std::holds_alternative<EdifyTokenWhitespace>(*it2)
                    || std::holds_alternative<EdifyTokenNewline>(*it2)
                    || std::holds_alternative<EdifyTokenComment>(*it2)) {
                continue;
            } else if (std::holds_alternative<EdifyTokenLeftParen>(*it2)) {
                found_left_paren = true;
                bounds.left_paren = it2;
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

        for (auto it2 = bounds.left_paren; it2 != end; ++it2) {
            if (std::holds_alternative<EdifyTokenLeftParen>(*it2)) {
                ++depth;
            } else if (std::holds_alternative<EdifyTokenRightParen>(*it2)) {
                --depth;
            }
            if (depth == 0) {
                found_right_paren = true;
                bounds.right_paren = it2;
                break;
            }
        }

        // If a right parenthesis was not found, but the function name and left
        // parenthesis were found, then assume there's a syntax error and bail
        // out
        if (!found_right_paren) {
            return {};
        }

        return bounds;
    }

    return {};
}

/*!
 * \brief Replace edify function
 *
 * \param tokens List of edify tokens
 * \param bounds Iterator bounds of the function to replace
 * \param replacement Replacement edify function (in string form)
 *
 * \return New iterator pointing to position *after* the right parenthesis of
 *         the replaced function. Returns tokens.end() if the replacement
 *         string could not be tokenized.
 */
static TokenIter
replace_function(std::vector<EdifyToken> &tokens,
                 const FunctionBounds &bounds, const std::string &replacement)
{
    auto ret = EdifyTokenizer::tokenize(replacement);
    if (!ret) {
        LOGE("Failed to tokenize replacement function string: %s: %s",
             replacement.c_str(), ret.error().message().c_str());
        return tokens.end();
    }
    auto &rtokens = ret.value();

    // Remove replaced tokens
    auto it = tokens.erase(bounds.func_name, bounds.right_paren + 1);

    // Add replacement tokens
    it = tokens.insert(it, rtokens.begin(), rtokens.end());

    // Move iterator to the end of the replaced tokens
    it += static_cast<ptrdiff_t>(rtokens.size());

    return it;
}

/*!
 * \brief Replace edify mount() command
 *
 * \param tokens List of edify tokens
 * \param bounds Iterator bounds of the function to replace
 * \param system_devs List of system partition block devices
 * \param cache_devs List of cache partition block devices
 * \param data_devs List of data partition block devices
 *
 * \return Iterator pointing to position immediately after the right parenthesis
 *         or tokens.end() if an error occurs.
 */
static TokenIter
replace_edify_mount(std::vector<EdifyToken> &tokens,
                    const FunctionBounds &bounds,
                    const std::vector<std::string> &system_devs,
                    const std::vector<std::string> &cache_devs,
                    const std::vector<std::string> &data_devs)
{
    // For the mount() edify function, replace with the corresponding
    // update-binary-tool command
    for (auto it = bounds.left_paren + 1; it != bounds.right_paren; ++it) {
        if (!std::holds_alternative<EdifyTokenString>(*it)) {
            continue;
        }

        auto const &token = std::get<EdifyTokenString>(*it);
        const std::string str = token.raw_string();

        bool is_system = str.find("/system") != std::string::npos
                || find_items_in_string(str.c_str(), system_devs);
        bool is_cache = str.find("/cache") != std::string::npos
                || find_items_in_string(str.c_str(), cache_devs);
        bool is_data = str.find("/data") != std::string::npos
                || str.find("/userdata") != std::string::npos
                || find_items_in_string(str.c_str(), data_devs);

        if (is_system) {
            return replace_function(tokens, bounds,
                                    format(MOUNT_FMT, "/system"));
        } else if (is_cache) {
            return replace_function(tokens, bounds,
                                    format(MOUNT_FMT, "/cache"));
        } else if (is_data) {
            return replace_function(tokens, bounds,
                                    format(MOUNT_FMT, "/data"));
        }
    }
    return bounds.right_paren + 1;
}

/*!
 * \brief Replace edify unmount() command
 *
 * \param tokens List of edify tokens
 * \param bounds Iterator bounds of the function to replace
 * \param system_devs List of system partition block devices
 * \param cache_devs List of cache partition block devices
 * \param data_devs List of data partition block devices
 *
 * \return Iterator pointing to position immediately after the right parenthesis
 *         or tokens.end() if an error occurs.
 */
static TokenIter
replace_edify_unmount(std::vector<EdifyToken> &tokens,
                      const FunctionBounds &bounds,
                      const std::vector<std::string> &system_devs,
                      const std::vector<std::string> &cache_devs,
                      const std::vector<std::string> &data_devs)
{
    // For the unmount() edify function, replace with the corresponding
    // update-binary-tool command
    for (auto it = bounds.left_paren + 1; it != bounds.right_paren; ++it) {
        if (!std::holds_alternative<EdifyTokenString>(*it)) {
            continue;
        }

        auto const &token = std::get<EdifyTokenString>(*it);
        const std::string str = token.raw_string();

        bool is_system = str.find("/system") != std::string::npos
                || find_items_in_string(str.c_str(), system_devs);
        bool is_cache = str.find("/cache") != std::string::npos
                || find_items_in_string(str.c_str(), cache_devs);
        bool is_data = str.find("/data") != std::string::npos
                || str.find("/userdata") != std::string::npos
                || find_items_in_string(str.c_str(), data_devs);

        if (is_system) {
            return replace_function(tokens, bounds,
                                    format(UNMOUNT_FMT, "/system"));
        } else if (is_cache) {
            return replace_function(tokens, bounds,
                                    format(UNMOUNT_FMT, "/cache"));
        } else if (is_data) {
            return replace_function(tokens, bounds,
                                    format(UNMOUNT_FMT, "/data"));
        }
    }
    return bounds.right_paren + 1;
}

/*!
 * \brief Replace edify run_program() command
 *
 * \param tokens List of edify tokens
 * \param bounds Iterator bounds of the function to replace
 * \param system_devs List of system partition block devices
 * \param cache_devs List of cache partition block devices
 * \param data_devs List of data partition block devices
 *
 * \return Iterator pointing to position immediately after the right parenthesis
 *         or tokens.end() if an error occurs.
 */
static TokenIter
replace_edify_run_program(std::vector<EdifyToken> &tokens,
                          const FunctionBounds &bounds,
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

    for (auto it = bounds.left_paren + 1; it != bounds.right_paren; ++it) {
        if (!std::holds_alternative<EdifyTokenString>(*it)) {
            continue;
        }

        auto const &token = std::get<EdifyTokenString>(*it);
        auto ret = token.unescaped_string();
        if (!ret) {
            LOGE("Failed to unescape string token: %s: %s",
                 token.raw_string().c_str(), ret.error().message().c_str());
            return tokens.end();
        }
        auto const &unescaped = ret.value();

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
        return replace_function(tokens, bounds,
                                "(ui_print(\"Removed reboot command\") == 0)");
    } else if (found_umount) {
        if (is_system) {
            return replace_function(tokens, bounds,
                                    format(UNMOUNT_FMT, "/system"));
        } else if (is_cache) {
            return replace_function(tokens, bounds,
                                    format(UNMOUNT_FMT, "/cache"));
        } else if (is_data) {
            return replace_function(tokens, bounds,
                                    format(UNMOUNT_FMT, "/data"));
        }
    } else if (found_mount) {
        if (is_system) {
            return replace_function(tokens, bounds,
                                    format(MOUNT_FMT, "/system"));
        } else if (is_cache) {
            return replace_function(tokens, bounds,
                                    format(MOUNT_FMT, "/cache"));
        } else if (is_data) {
            return replace_function(tokens, bounds,
                                    format(MOUNT_FMT, "/data"));
        }
    } else if (found_format_sh) {
        return replace_function(tokens, bounds,
                                format(FORMAT_FMT, "/system"));
    } else if (found_mke2fs) {
        if (is_system) {
            return replace_function(tokens, bounds,
                                    format(FORMAT_FMT, "/system"));
        } else if (is_cache) {
            return replace_function(tokens, bounds,
                                    format(FORMAT_FMT, "/cache"));
        } else if (is_data) {
            return replace_function(tokens, bounds,
                                    format(FORMAT_FMT, "/data"));
        }
    }

    return bounds.right_paren + 1;
}

/*!
 * \brief Replace edify delete_recursive() command
 *
 * \param tokens List of edify tokens
 * \param bounds Iterator bounds of the function to replace
 *
 * \return Iterator pointing to position immediately after the right parenthesis
 *         or tokens.end() if an error occurs.
 */
static TokenIter
replace_edify_delete_recursive(std::vector<EdifyToken> &tokens,
                               const FunctionBounds &bounds)
{
    for (auto it = bounds.left_paren + 1; it != bounds.right_paren; ++it) {
        if (!std::holds_alternative<EdifyTokenString>(*it)) {
            continue;
        }

        auto const &token = std::get<EdifyTokenString>(*it);
        auto ret = token.unescaped_string();
        if (!ret) {
            LOGE("Failed to unescape string token: %s: %s",
                 token.raw_string().c_str(), ret.error().message().c_str());
            return tokens.end();
        }
        auto const &unescaped = ret.value();

        if (unescaped == "/system" || unescaped == "/system/") {
            return replace_function(tokens, bounds,
                                    format(FORMAT_FMT, "/system"));
        } else if (unescaped == "/cache" || unescaped == "/cache/") {
            return replace_function(tokens, bounds,
                                    format(FORMAT_FMT, "/cache"));
        }
    }
    return bounds.right_paren + 1;
}

/*!
 * \brief Replace edify format() command
 *
 * \param tokens List of edify tokens
 * \param bounds Iterator bounds of the function to replace
 * \param system_devs List of system partition block devices
 * \param cache_devs List of cache partition block devices
 * \param data_devs List of data partition block devices
 *
 * \return Iterator pointing to position immediately after the right parenthesis
 *         or tokens.end() if an error occurs.
 */
static TokenIter
replace_edify_format(std::vector<EdifyToken> &tokens,
                     const FunctionBounds &bounds,
                     const std::vector<std::string> &system_devs,
                     const std::vector<std::string> &cache_devs,
                     const std::vector<std::string> &data_devs)
{
    // For the format() edify function, replace with the corresponding
    // update-binary-tool command
    for (auto it = bounds.left_paren + 1; it != bounds.right_paren; ++it) {
        if (!std::holds_alternative<EdifyTokenString>(*it)) {
            continue;
        }

        auto const &token = std::get<EdifyTokenString>(*it);
        const std::string str = token.raw_string();

        bool is_system = str.find("/system") != std::string::npos
                || find_items_in_string(str.c_str(), system_devs);
        bool is_cache = str.find("/cache") != std::string::npos
                || find_items_in_string(str.c_str(), cache_devs);
        bool is_data = str.find("/data") != std::string::npos
                || str.find("/userdata") != std::string::npos
                || find_items_in_string(str.c_str(), data_devs);

        if (is_system) {
            return replace_function(tokens, bounds,
                                    format(FORMAT_FMT, "/system"));
        } else if (is_cache) {
            return replace_function(tokens, bounds,
                                    format(FORMAT_FMT, "/cache"));
        } else if (is_data) {
            return replace_function(tokens, bounds,
                                    format(FORMAT_FMT, "/data"));
        }
    }
    return bounds.right_paren + 1;
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
    std::string contents;
    std::string path;

    path += directory;
    path += "/";
    path += UpdaterScript;

    FileUtils::read_to_string(path, &contents);

    if (starts_with(contents, "#!")) {
        // Ignore any script with a shebang line
        return true;
    }

    auto ret = EdifyTokenizer::tokenize(contents);
    if (!ret) {
        LOGE("Failed to tokenize updater-script: %s",
             ret.error().message().c_str());
        return false;
    }
    auto &tokens = ret.value();

#if DUMP_DEBUG
    EdifyTokenizer::dump(tokens);
#endif

    auto &&device = m_info.device();
    auto system_devs = device.system_block_devs();
    auto cache_devs = device.cache_block_devs();
    auto data_devs = device.data_block_devs();

    TokenIter begin = tokens.begin();
    TokenIter end;

    while (true) {
        end = tokens.end();

        // Need to find:
        // 1. String containing function name
        // 2. Left parenthesis for the function
        // 3. Right parenthesis for the function
        auto bounds = find_function(begin, end);
        if (!bounds) {
            break;
        }

        // Tokens (types are checked by findFunction())
        auto const &t_func_name = std::get<EdifyTokenString>(*bounds->func_name);
        auto unescaped = t_func_name.unescaped_string();
        if (!unescaped) {
            LOGE("Failed to unescape string token: %s: %s",
                 t_func_name.raw_string().c_str(),
                 unescaped.error().message().c_str());
            return false;
        }

        if (unescaped.value() == "mount") {
            begin = replace_edify_mount(tokens, *bounds,
                                        system_devs, cache_devs, data_devs);
        } else if (unescaped.value() == "unmount") {
            begin = replace_edify_unmount(tokens, *bounds,
                                          system_devs, cache_devs, data_devs);
        } else if (unescaped.value() == "run_program") {
            begin = replace_edify_run_program(tokens, *bounds,
                                              system_devs, cache_devs, data_devs);
        } else if (unescaped.value() == "delete_recursive") {
            begin = replace_edify_delete_recursive(tokens, *bounds);
        } else if (unescaped.value() == "format") {
            begin = replace_edify_format(tokens, *bounds,
                                         system_devs, cache_devs, data_devs);
        } else {
            // Only skip function name so that we catch nested function calls
            begin = bounds->func_name + 1;
        }

        if (begin == tokens.end()) {
            return false;
        }
    }

#if DUMP_DEBUG
    EdifyTokenizer::dump(tokens);
#endif

    FileUtils::write_from_string(path, EdifyTokenizer::untokenize(tokens));

    return true;
}

bool StandardPatcher::patch_transfer_list(const std::string &directory)
{
    std::string contents;
    std::string path;

    path += directory;
    path += "/";
    path += SystemTransferList;

    auto ret = FileUtils::read_to_string(path, &contents);
    if (ret != ErrorCode::NoError) {
        return ret == ErrorCode::FileOpenError;
    }

    auto lines = split_sv(contents, '\n');

    for (auto it = lines.begin(); it != lines.end();) {
        if (starts_with(*it, "erase ")) {
            it = lines.erase(it);
        } else {
            ++it;
        }
    }

    contents = join(lines, "\n");
    FileUtils::write_from_string(path, contents);

    return true;
}

}
