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
    std::string data_dir;
    std::string temp_dir;

    // Errors
    ErrorCode error;

    // Created patchers
    std::vector<Patcher *> alloc_patchers;
    std::vector<AutoPatcher *> alloc_auto_patchers;
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

    for (Patcher *patcher : priv->alloc_patchers) {
        destroy_patcher(patcher);
    }
    priv->alloc_patchers.clear();

    for (AutoPatcher *patcher : priv->alloc_auto_patchers) {
        destroy_auto_patcher(patcher);
    }
    priv->alloc_auto_patchers.clear();
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
std::string PatcherConfig::data_directory() const
{
    MB_PRIVATE(const PatcherConfig);
    return priv->data_dir;
}

/*!
 * \brief Get the temporary directory
 *
 * The default directory is the system's (OS-dependent) temporary directory.
 *
 * \return Temporary directory
 */
std::string PatcherConfig::temp_directory() const
{
    MB_PRIVATE(const PatcherConfig);

    if (priv->temp_dir.empty()) {
        return FileUtils::system_temporary_dir();
    } else {
        return priv->temp_dir;
    }
}

/*!
 * \brief Set top-level data directory
 *
 * \param path Path to data directory
 */
void PatcherConfig::set_data_directory(std::string path)
{
    MB_PRIVATE(PatcherConfig);
    priv->data_dir = std::move(path);
}

/*!
 * \brief Set the temporary directory
 *
 * \note This should only be changed if the system's temporary directory is not
 *       desired.
 *
 * \param path Path to temporary directory
 */
void PatcherConfig::set_temp_directory(std::string path)
{
    MB_PRIVATE(PatcherConfig);
    priv->temp_dir = std::move(path);
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
std::vector<std::string> PatcherConfig::auto_patchers() const
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
Patcher * PatcherConfig::create_patcher(const std::string &id)
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
        priv->alloc_patchers.push_back(p);
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
AutoPatcher * PatcherConfig::create_auto_patcher(const std::string &id,
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
        priv->alloc_auto_patchers.push_back(ap);
    }

    return ap;
}

/*!
 * \brief Destroys a Patcher and frees its memory
 *
 * \param patcher Patcher to destroy
 */
void PatcherConfig::destroy_patcher(Patcher *patcher)
{
    MB_PRIVATE(PatcherConfig);

    auto it = std::find(priv->alloc_patchers.begin(),
                        priv->alloc_patchers.end(),
                        patcher);

    assert(it != priv->alloc_patchers.end());

    priv->alloc_patchers.erase(it);
    delete patcher;
}

/*!
 * \brief Destroys an AutoPatcher and frees its memory
 *
 * \param patcher AutoPatcher to destroy
 */
void PatcherConfig::destroy_auto_patcher(AutoPatcher *patcher)
{
    MB_PRIVATE(PatcherConfig);

    auto it = std::find(priv->alloc_auto_patchers.begin(),
                        priv->alloc_auto_patchers.end(),
                        patcher);

    assert(it != priv->alloc_auto_patchers.end());

    priv->alloc_auto_patchers.erase(it);
    delete patcher;
}

}
}
