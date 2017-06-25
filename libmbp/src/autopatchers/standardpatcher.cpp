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

#include "mbp/autopatchers/standardpatcher.h"

#include <cstring>

#include "mbcommon/string.h"
#include "mblog/logging.h"

#include "mbp/edify/tokenizer.h"
#include "mbp/private/fileutils.h"
#include "mbp/private/stringutils.h"

#define DUMP_DEBUG 0


namespace mbp
{

/*! \cond INTERNAL */
class StandardPatcher::Impl
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

#define MOUNT_FMT \
        "(run_program(\"/update-binary-tool\", \"mount\", \"%s\") == 0)"
#define UNMOUNT_FMT \
        "(run_program(\"/update-binary-tool\", \"unmount\", \"%s\") == 0)"
#define FORMAT_FMT \
        "(run_program(\"/update-binary-tool\", \"format\", \"%s\") == 0)"


StandardPatcher::StandardPatcher(const PatcherConfig * const pc,
                                 const FileInfo * const info) :
    m_impl(new Impl())
{
    m_impl->pc = pc;
    m_impl->info = info;
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

std::vector<std::string> StandardPatcher::newFiles() const
{
    return std::vector<std::string>();
}

std::vector<std::string> StandardPatcher::existingFiles() const
{
    return { UpdaterScript, SystemTransferList };
}

static bool findItemsInString(const char *haystack,
                              const char * const *needles)
{
    for (auto iter = needles; *iter; ++iter) {
        if (strstr(haystack, *iter)) {
            return true;
        }
    }

    return false;
}

static bool findFunction(const std::vector<EdifyToken *>::iterator begin,
                         const std::vector<EdifyToken *>::iterator end,
                         std::vector<EdifyToken *>::iterator *outFuncName,
                         std::vector<EdifyToken *>::iterator *outLeftParen,
                         std::vector<EdifyToken *>::iterator *outRightParen)
{
    std::vector<EdifyToken *>::iterator funcName;
    std::vector<EdifyToken *>::iterator leftParen;
    std::vector<EdifyToken *>::iterator rightParen;

    for (auto it = begin; it != end; ++it) {
        // Find string representing the function name
        if ((*it)->type() != EdifyTokenType::String) {
            continue;
        }

        funcName = it;

        bool foundLeftParen = false;
        bool foundRightParen = false;

        // Barring any whitespace, newlines, or comments, the function name
        // should be followed by a left parenthesis
        for (auto it2 = it + 1; it2 != end; ++it2) {
            if ((*it2)->type() == EdifyTokenType::Whitespace
                    || (*it2)->type() == EdifyTokenType::Newline
                    || (*it2)->type() == EdifyTokenType::Comment) {
                continue;
            } else if ((*it2)->type() == EdifyTokenType::LeftParen) {
                foundLeftParen = true;
                leftParen = it2;
            }
            break;
        }

        // If a left parenthesis was not found, then the string token was not
        // a function name
        if (!foundLeftParen) {
            continue;
        }

        // Left for matching right parenthesis
        std::size_t depth = 0;

        for (auto it2 = leftParen; it2 != end; ++it2) {
            if ((*it2)->type() == EdifyTokenType::LeftParen) {
                ++depth;
            } else if ((*it2)->type() == EdifyTokenType::RightParen) {
                --depth;
            }
            if (depth == 0) {
                foundRightParen = true;
                rightParen = it2;
                break;
            }
        }

        // If a right parenthesis was not found, but the function name and left
        // parenthesis were found, then assume there's a syntax error and bail
        // out
        if (!foundRightParen) {
            return false;
        }

        *outFuncName = funcName;
        *outLeftParen = leftParen;
        *outRightParen = rightParen;

        return true;
    }

    return false;
}

/*!
 * \brief Replace edify function
 *
 * \param tokens List of edify tokens
 * \param funcName Function name token of the replaced function
 * \param leftParen Left parenthesis token of the replaced function
 * \param rightParen Right parenthesis token of the replaced function
 * \param replacement Replacement edify function (in string form)
 *
 * \return New iterator pointing to position *after* the right parenthesis of
 *         the replaced function. Returns tokens->end() if the replacement
 *         string could not be tokenized.
 */
static std::vector<EdifyToken *>::iterator
replaceFunction(std::vector<EdifyToken *> *tokens,
                std::vector<EdifyToken *>::iterator funcName,
                std::vector<EdifyToken *>::iterator leftParen,
                std::vector<EdifyToken *>::iterator rightParen,
                const std::string &replacement)
{
    // Included for completeness' sake
    (void) leftParen;

    std::vector<EdifyToken *> replacementTokens;
    bool result = EdifyTokenizer::tokenize(
            replacement.data(), replacement.size(), &replacementTokens);
    if (!result) {
        LOGE("Failed to tokenize replacement function string: %s",
             replacement.c_str());
        return tokens->end();
    }

    // Deallocate replaced tokens
    for (auto it = funcName; it != rightParen; ++it) {
        delete *it;
    }
    delete *rightParen;

    // Remove replaced tokens
    auto it = tokens->erase(funcName, rightParen);
    it = tokens->erase(it);

    // Add replacement tokens
    it = tokens->insert(it, replacementTokens.begin(), replacementTokens.end());

    // Move iterator to the end of the replaced tokens
    it += replacementTokens.size();

    return it;
}

/*!
 * \brief Replace edify mount() command
 *
 * \param tokens List of edify tokens
 * \param funcName Function name token
 * \param leftParen Left parenthesis token
 * \param rightParen Right parenthesis token
 * \param systemDevs List of system partition block devices
 * \param cacheDevs List of cache partition block devices
 * \param dataDevs List of data partition block devices
 *
 * \return Iterator pointing to position immediately after the right parenthesis
 */
static std::vector<EdifyToken *>::iterator
replaceEdifyMount(std::vector<EdifyToken *> *tokens,
                  const std::vector<EdifyToken *>::iterator funcName,
                  const std::vector<EdifyToken *>::iterator leftParen,
                  const std::vector<EdifyToken *>::iterator rightParen,
                  const char * const *systemDevs,
                  const char * const *cacheDevs,
                  const char * const *dataDevs)
{
    // For the mount() edify function, replace with the corresponding
    // update-binary-tool command
    for (auto it = leftParen + 1; it != rightParen; ++it) {
        if ((*it)->type() != EdifyTokenType::String) {
            continue;
        }

        EdifyTokenString *token = (EdifyTokenString *)(*it);
        const std::string str = token->string();

        bool isSystem = str.find("/system") != std::string::npos
                || findItemsInString(str.c_str(), systemDevs);
        bool isCache = str.find("/cache") != std::string::npos
                || findItemsInString(str.c_str(), cacheDevs);
        bool isData = str.find("/data") != std::string::npos
                || str.find("/userdata") != std::string::npos
                || findItemsInString(str.c_str(), dataDevs);

        if (isSystem) {
            return replaceFunction(tokens, funcName, leftParen, rightParen,
                                   StringUtils::format(MOUNT_FMT, "/system"));
        } else if (isCache) {
            return replaceFunction(tokens, funcName, leftParen, rightParen,
                                   StringUtils::format(MOUNT_FMT, "/cache"));
        } else if (isData) {
            return replaceFunction(tokens, funcName, leftParen, rightParen,
                                   StringUtils::format(MOUNT_FMT, "/data"));
        }
    }
    return rightParen + 1;
}

/*!
 * \brief Replace edify unmount() command
 *
 * \param tokens List of edify tokens
 * \param funcName Function name token
 * \param leftParen Left parenthesis token
 * \param rightParen Right parenthesis token
 * \param systemDevs List of system partition block devices
 * \param cacheDevs List of cache partition block devices
 * \param dataDevs List of data partition block devices
 *
 * \return Iterator pointing to position immediately after the right parenthesis
 */
static std::vector<EdifyToken *>::iterator
replaceEdifyUnmount(std::vector<EdifyToken *> *tokens,
                    const std::vector<EdifyToken *>::iterator funcName,
                    const std::vector<EdifyToken *>::iterator leftParen,
                    const std::vector<EdifyToken *>::iterator rightParen,
                    const char * const *systemDevs,
                    const char * const *cacheDevs,
                    const char * const *dataDevs)
{
    // For the unmount() edify function, replace with the corresponding
    // update-binary-tool command
    for (auto it = leftParen + 1; it != rightParen; ++it) {
        if ((*it)->type() != EdifyTokenType::String) {
            continue;
        }

        EdifyTokenString *token = (EdifyTokenString *)(*it);
        const std::string str = token->string();

        bool isSystem = str.find("/system") != std::string::npos
                || findItemsInString(str.c_str(), systemDevs);
        bool isCache = str.find("/cache") != std::string::npos
                || findItemsInString(str.c_str(), cacheDevs);
        bool isData = str.find("/data") != std::string::npos
                || str.find("/userdata") != std::string::npos
                || findItemsInString(str.c_str(), dataDevs);

        if (isSystem) {
            return replaceFunction(tokens, funcName, leftParen, rightParen,
                                   StringUtils::format(UNMOUNT_FMT, "/system"));
        } else if (isCache) {
            return replaceFunction(tokens, funcName, leftParen, rightParen,
                                   StringUtils::format(UNMOUNT_FMT, "/cache"));
        } else if (isData) {
            return replaceFunction(tokens, funcName, leftParen, rightParen,
                                   StringUtils::format(UNMOUNT_FMT, "/data"));
        }
    }
    return rightParen + 1;
}

/*!
 * \brief Replace edify run_program() command
 *
 * \param tokens List of edify tokens
 * \param funcName Function name token
 * \param leftParen Left parenthesis token
 * \param rightParen Right parenthesis token
 * \param systemDevs List of system partition block devices
 * \param cacheDevs List of cache partition block devices
 * \param dataDevs List of data partition block devices
 *
 * \return Iterator pointing to position immediately after the right parenthesis
 */
static std::vector<EdifyToken *>::iterator
replaceEdifyRunProgram(std::vector<EdifyToken *> *tokens,
                       const std::vector<EdifyToken *>::iterator funcName,
                       const std::vector<EdifyToken *>::iterator leftParen,
                       const std::vector<EdifyToken *>::iterator rightParen,
                       const char * const *systemDevs,
                       const char * const *cacheDevs,
                       const char * const *dataDevs)
{
    bool foundReboot = false;
    bool foundMount = false;
    bool foundUmount = false;
    bool foundFormatSh = false;
    bool foundMke2fs = false;
    bool isSystem = false;
    bool isCache = false;
    bool isData = false;

    for (auto it = leftParen + 1; it != rightParen; ++it) {
        if ((*it)->type() != EdifyTokenType::String) {
            continue;
        }

        EdifyTokenString *token = (EdifyTokenString *)(*it);
        const std::string unescaped = token->unescapedString();

        if (mb_ends_with(unescaped.c_str(), "reboot")) {
            foundReboot = true;
        }
        if (mb_ends_with(unescaped.c_str(), "mount")) {
            foundMount = true;
        }
        if (mb_ends_with(unescaped.c_str(), "umount")) {
            foundUmount = true;
        }
        if (mb_ends_with(unescaped.c_str(), "/format.sh")) {
            foundFormatSh = true;
        }
        if (mb_ends_with(unescaped.c_str(), "/mke2fs")) {
            foundMke2fs = true;
        }

        if (unescaped.find("/system") != std::string::npos
                || findItemsInString(unescaped.c_str(), systemDevs)) {
            isSystem = true;
        }
        if (unescaped.find("/cache") != std::string::npos
                || findItemsInString(unescaped.c_str(), cacheDevs)) {
            isCache = true;
        }
        if (unescaped.find("/data") != std::string::npos
                || unescaped.find("/userdata") != std::string::npos
                || findItemsInString(unescaped.c_str(), dataDevs)) {
            isData = true;
        }
    }

    if (foundReboot) {
        return replaceFunction(tokens, funcName, leftParen, rightParen,
                               "(ui_print(\"Removed reboot command\") == 0)");
    } else if (foundUmount) {
        if (isSystem) {
            return replaceFunction(tokens, funcName, leftParen, rightParen,
                                   StringUtils::format(UNMOUNT_FMT, "/system"));
        } else if (isCache) {
            return replaceFunction(tokens, funcName, leftParen, rightParen,
                                   StringUtils::format(UNMOUNT_FMT, "/cache"));
        } else if (isData) {
            return replaceFunction(tokens, funcName, leftParen, rightParen,
                                   StringUtils::format(UNMOUNT_FMT, "/data"));
        }
    } else if (foundMount) {
        if (isSystem) {
            return replaceFunction(tokens, funcName, leftParen, rightParen,
                                   StringUtils::format(MOUNT_FMT, "/system"));
        } else if (isCache) {
            return replaceFunction(tokens, funcName, leftParen, rightParen,
                                   StringUtils::format(MOUNT_FMT, "/cache"));
        } else if (isData) {
            return replaceFunction(tokens, funcName, leftParen, rightParen,
                                   StringUtils::format(MOUNT_FMT, "/data"));
        }
    } else if (foundFormatSh) {
        return replaceFunction(tokens, funcName, leftParen, rightParen,
                               StringUtils::format(FORMAT_FMT, "/system"));
    } else if (foundMke2fs) {
        if (isSystem) {
            return replaceFunction(tokens, funcName, leftParen, rightParen,
                                   StringUtils::format(FORMAT_FMT, "/system"));
        } else if (isCache) {
            return replaceFunction(tokens, funcName, leftParen, rightParen,
                                   StringUtils::format(FORMAT_FMT, "/cache"));
        } else if (isData) {
            return replaceFunction(tokens, funcName, leftParen, rightParen,
                                   StringUtils::format(FORMAT_FMT, "/data"));
        }
    }

    return rightParen + 1;
}

/*!
 * \brief Replace edify delete_recursive() command
 *
 * \param tokens List of edify tokens
 * \param funcName Function name token
 * \param leftParen Left parenthesis token
 * \param rightParen Right parenthesis token
 *
 * \return Iterator pointing to position immediately after the right parenthesis
 */
static std::vector<EdifyToken *>::iterator
replaceEdifyDeleteRecursive(std::vector<EdifyToken *> *tokens,
                            const std::vector<EdifyToken *>::iterator funcName,
                            const std::vector<EdifyToken *>::iterator leftParen,
                            const std::vector<EdifyToken *>::iterator rightParen)
{
    for (auto it = leftParen + 1; it != rightParen; ++it) {
        if ((*it)->type() != EdifyTokenType::String) {
            continue;
        }

        EdifyTokenString *token = (EdifyTokenString *)(*it);
        const std::string unescaped = token->unescapedString();

        if (unescaped == "/system" || unescaped == "/system/") {
            return replaceFunction(tokens, funcName, leftParen, rightParen,
                                   StringUtils::format(FORMAT_FMT, "/system"));
        } else if (unescaped == "/cache" || unescaped == "/cache/") {
            return replaceFunction(tokens, funcName, leftParen, rightParen,
                                   StringUtils::format(FORMAT_FMT, "/cache"));
        }
    }
    return rightParen + 1;
}

/*!
 * \brief Replace edify format() command
 *
 * \param tokens List of edify tokens
 * \param funcName Function name token
 * \param leftParen Left parenthesis token
 * \param rightParen Right parenthesis token
 * \param systemDevs List of system partition block devices
 * \param cacheDevs List of cache partition block devices
 * \param dataDevs List of data partition block devices
 *
 * \return Iterator pointing to position immediately after the right parenthesis
 */
static std::vector<EdifyToken *>::iterator
replaceEdifyFormat(std::vector<EdifyToken *> *tokens,
                   const std::vector<EdifyToken *>::iterator funcName,
                   const std::vector<EdifyToken *>::iterator leftParen,
                   const std::vector<EdifyToken *>::iterator rightParen,
                   const char * const *systemDevs,
                   const char * const *cacheDevs,
                   const char * const *dataDevs)
{
    // For the format() edify function, replace with the corresponding
    // update-binary-tool command
    for (auto it = leftParen + 1; it != rightParen; ++it) {
        if ((*it)->type() != EdifyTokenType::String) {
            continue;
        }

        EdifyTokenString *token = (EdifyTokenString *)(*it);
        const std::string str = token->string();

        bool isSystem = str.find("/system") != std::string::npos
                || findItemsInString(str.c_str(), systemDevs);
        bool isCache = str.find("/cache") != std::string::npos
                || findItemsInString(str.c_str(), cacheDevs);
        bool isData = str.find("/data") != std::string::npos
                || str.find("/userdata") != std::string::npos
                || findItemsInString(str.c_str(), dataDevs);

        if (isSystem) {
            return replaceFunction(tokens, funcName, leftParen, rightParen,
                                   StringUtils::format(FORMAT_FMT, "/system"));
        } else if (isCache) {
            return replaceFunction(tokens, funcName, leftParen, rightParen,
                                   StringUtils::format(FORMAT_FMT, "/cache"));
        } else if (isData) {
            return replaceFunction(tokens, funcName, leftParen, rightParen,
                                   StringUtils::format(FORMAT_FMT, "/data"));
        }
    }
    return rightParen + 1;
}

bool StandardPatcher::patchFiles(const std::string &directory)
{
    if (!patchUpdater(directory)) {
        return false;
    }

    if (!patchTransferList(directory)) {
        return false;
    }

    return true;
}

bool StandardPatcher::patchUpdater(const std::string &directory)
{
    std::string contents;
    std::string path;

    path += directory;
    path += "/";
    path += UpdaterScript;

    FileUtils::readToString(path, &contents);

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

    Device *device = m_impl->info->device();
    auto systemDevs = mb_device_system_block_devs(device);
    auto cacheDevs = mb_device_cache_block_devs(device);
    auto dataDevs = mb_device_data_block_devs(device);

    std::vector<EdifyToken *>::iterator begin = tokens.begin();
    std::vector<EdifyToken *>::iterator end;

    // TODO: Catch errors
    while (true) {
        end = tokens.end();

        // Need to find:
        // 1. String containing function name
        // 2. Left parenthesis for the function
        // 3. Right parenthesis for the function
        std::vector<EdifyToken *>::iterator funcName;
        std::vector<EdifyToken *>::iterator leftParen;
        std::vector<EdifyToken *>::iterator rightParen;

        if (!findFunction(begin, end, &funcName, &leftParen, &rightParen)) {
            break;
        }

        // Tokens (types are checked by findFunction())
        EdifyTokenString *tFuncName = (EdifyTokenString *)(*funcName);

        if (tFuncName->unescapedString() == "mount") {
            begin = replaceEdifyMount(&tokens, funcName, leftParen, rightParen,
                                      systemDevs, cacheDevs, dataDevs);
        } else if (tFuncName->unescapedString() == "unmount") {
            begin = replaceEdifyUnmount(&tokens, funcName, leftParen, rightParen,
                                        systemDevs, cacheDevs, dataDevs);
        } else if (tFuncName->unescapedString() == "run_program") {
            begin = replaceEdifyRunProgram(&tokens, funcName, leftParen, rightParen,
                                           systemDevs, cacheDevs, dataDevs);
        } else if (tFuncName->unescapedString() == "delete_recursive") {
            begin = replaceEdifyDeleteRecursive(&tokens, funcName, leftParen, rightParen);
        } else if (tFuncName->unescapedString() == "format") {
            begin = replaceEdifyFormat(&tokens, funcName, leftParen, rightParen,
                                       systemDevs, cacheDevs, dataDevs);
        } else {
            begin = funcName + 1;
        }
    }

#if DUMP_DEBUG
    EdifyTokenizer::dump(tokens);
#endif

    FileUtils::writeFromString(path, EdifyTokenizer::untokenize(tokens));

    for (EdifyToken *t : tokens) {
        delete t;
    }

    return true;
}

bool StandardPatcher::patchTransferList(const std::string &directory)
{
    std::string contents;
    std::string path;
    std::vector<std::string> lines;

    path += directory;
    path += "/";
    path += SystemTransferList;

    auto ret = FileUtils::readToString(path, &contents);
    if (ret != ErrorCode::NoError) {
        return ret == ErrorCode::FileOpenError;
    }

    lines = StringUtils::split(contents, '\n');

    for (auto it = lines.begin(); it != lines.end();) {
        if (mb_starts_with(it->c_str(), "erase ")) {
            it = lines.erase(it);
        } else {
            ++it;
        }
    }

    contents = StringUtils::join(lines, "\n");
    FileUtils::writeFromString(path, contents);

    return true;
}

}
