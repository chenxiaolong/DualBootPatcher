/*
 * Copyright (C) 2015-2017  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "mbpatcher/patcherconfig.h"

#include <algorithm>

#include <cassert>

#include "mbpatcher/patcherinterface.h"
#include "mbpatcher/private/fileutils.h"

// Patchers
#include "mbpatcher/autopatchers/standardpatcher.h"
#include "mbpatcher/autopatchers/mountcmdpatcher.h"
#include "mbpatcher/patchers/odinpatcher.h"
#include "mbpatcher/patchers/ramdiskupdater.h"
#include "mbpatcher/patchers/zippatcher.h"


namespace mb
{
namespace patcher
{

/*! \cond INTERNAL */
class PatcherConfigPrivate
{
public:
    // Directories
    std::string dataDir;
    std::string tempDir;

    // Errors
    ErrorCode error;

    // Created patchers
    std::vector<Patcher *> allocPatchers;
    std::vector<AutoPatcher *> allocAutoPatchers;
};
/*! \endcond */

/*!
 * \class PatcherConfig
 *
 * This is the main interface of the patcher.
 * Blah blah documenting later ;)
 */

PatcherConfig::PatcherConfig() : _priv_ptr(new PatcherConfigPrivate())
{
}

PatcherConfig::~PatcherConfig()
{
    MB_PRIVATE(PatcherConfig);

    for (Patcher *patcher : priv->allocPatchers) {
        destroyPatcher(patcher);
    }
    priv->allocPatchers.clear();

    for (AutoPatcher *patcher : priv->allocAutoPatchers) {
        destroyAutoPatcher(patcher);
    }
    priv->allocAutoPatchers.clear();
}

/*!
 * \brief Get error information
 *
 * \note The returned ErrorCode contains valid information only if an
 *       operation has failed.
 *
 * \return ErrorCode containing information about the error
 */
ErrorCode PatcherConfig::error() const
{
    MB_PRIVATE(const PatcherConfig);
    return priv->error;
}

/*!
 * \brief Get top-level data directory
 *
 * \return Data directory
 */
std::string PatcherConfig::dataDirectory() const
{
    MB_PRIVATE(const PatcherConfig);
    return priv->dataDir;
}

/*!
 * \brief Get the temporary directory
 *
 * The default directory is the system's (OS-dependent) temporary directory.
 *
 * \return Temporary directory
 */
std::string PatcherConfig::tempDirectory() const
{
    MB_PRIVATE(const PatcherConfig);

    if (priv->tempDir.empty()) {
        return FileUtils::systemTemporaryDir();
    } else {
        return priv->tempDir;
    }
}

/*!
 * \brief Set top-level data directory
 *
 * \param path Path to data directory
 */
void PatcherConfig::setDataDirectory(std::string path)
{
    MB_PRIVATE(PatcherConfig);
    priv->dataDir = std::move(path);
}

/*!
 * \brief Set the temporary directory
 *
 * \note This should only be changed if the system's temporary directory is not
 *       desired.
 *
 * \param path Path to temporary directory
 */
void PatcherConfig::setTempDirectory(std::string path)
{
    MB_PRIVATE(PatcherConfig);
    priv->tempDir = std::move(path);
}

/*!
 * \brief Get list of Patcher IDs
 *
 * \return List of Patcher names
 */
std::vector<std::string> PatcherConfig::patchers() const
{
    return {
        OdinPatcher::Id,
        RamdiskUpdater::Id,
        ZipPatcher::Id,
    };
}

/*!
 * \brief Get list of AutoPatcher IDs
 *
 * \return List of AutoPatcher names
 */
std::vector<std::string> PatcherConfig::autoPatchers() const
{
    return {
        StandardPatcher::Id,
        MountCmdPatcher::Id
    };
}

/*!
 * \brief Create new Patcher
 *
 * \param id Patcher ID
 *
 * \return New Patcher
 */
Patcher * PatcherConfig::createPatcher(const std::string &id)
{
    MB_PRIVATE(PatcherConfig);

    Patcher *p = nullptr;

    if (id == OdinPatcher::Id) {
        p = new OdinPatcher(this);
    } else if (id == RamdiskUpdater::Id) {
        p = new RamdiskUpdater(this);
    } else if (id == ZipPatcher::Id) {
        p = new ZipPatcher(this);
    }

    if (p != nullptr) {
        priv->allocPatchers.push_back(p);
    }

    return p;
}

/*!
 * \brief Create new AutoPatcher
 *
 * \param id AutoPatcher ID
 * \param info FileInfo describing file to be patched
 *
 * \return New AutoPatcher
 */
AutoPatcher * PatcherConfig::createAutoPatcher(const std::string &id,
                                               const FileInfo * const info)
{
    MB_PRIVATE(PatcherConfig);

    AutoPatcher *ap = nullptr;

    if (id == StandardPatcher::Id) {
        ap = new StandardPatcher(this, info);
    } else if (id == MountCmdPatcher::Id) {
        ap = new MountCmdPatcher(this, info);
    }

    if (ap != nullptr) {
        priv->allocAutoPatchers.push_back(ap);
    }

    return ap;
}

/*!
 * \brief Destroys a Patcher and frees its memory
 *
 * \param patcher Patcher to destroy
 */
void PatcherConfig::destroyPatcher(Patcher *patcher)
{
    MB_PRIVATE(PatcherConfig);

    auto it = std::find(priv->allocPatchers.begin(),
                        priv->allocPatchers.end(),
                        patcher);

    assert(it != priv->allocPatchers.end());

    priv->allocPatchers.erase(it);
    delete patcher;
}

/*!
 * \brief Destroys an AutoPatcher and frees its memory
 *
 * \param patcher AutoPatcher to destroy
 */
void PatcherConfig::destroyAutoPatcher(AutoPatcher *patcher)
{
    MB_PRIVATE(PatcherConfig);

    auto it = std::find(priv->allocAutoPatchers.begin(),
                        priv->allocAutoPatchers.end(),
                        patcher);

    assert(it != priv->allocAutoPatchers.end());

    priv->allocAutoPatchers.erase(it);
    delete patcher;
}

}
}
