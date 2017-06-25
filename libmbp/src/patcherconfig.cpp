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

#include "mbp/patcherconfig.h"

#include <algorithm>

#include <cassert>

#include "mbp/patcherinterface.h"
#include "mbp/private/fileutils.h"

// Patchers
#include "mbp/autopatchers/standardpatcher.h"
#include "mbp/autopatchers/mountcmdpatcher.h"
#include "mbp/patchers/multibootpatcher.h"
#include "mbp/patchers/odinpatcher.h"
#include "mbp/patchers/ramdiskupdater.h"


namespace mbp
{

/*! \cond INTERNAL */
class PatcherConfig::Impl
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

PatcherConfig::PatcherConfig() : m_impl(new Impl())
{
}

PatcherConfig::~PatcherConfig()
{
    for (Patcher *patcher : m_impl->allocPatchers) {
        destroyPatcher(patcher);
    }
    m_impl->allocPatchers.clear();

    for (AutoPatcher *patcher : m_impl->allocAutoPatchers) {
        destroyAutoPatcher(patcher);
    }
    m_impl->allocAutoPatchers.clear();
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
    return m_impl->error;
}

/*!
 * \brief Get top-level data directory
 *
 * \return Data directory
 */
std::string PatcherConfig::dataDirectory() const
{
    return m_impl->dataDir;
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
    if (m_impl->tempDir.empty()) {
        return FileUtils::systemTemporaryDir();
    } else {
        return m_impl->tempDir;
    }
}

/*!
 * \brief Set top-level data directory
 *
 * \param path Path to data directory
 */
void PatcherConfig::setDataDirectory(std::string path)
{
    m_impl->dataDir = std::move(path);
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
    m_impl->tempDir = std::move(path);
}

/*!
 * \brief Get list of Patcher IDs
 *
 * \return List of Patcher names
 */
std::vector<std::string> PatcherConfig::patchers() const
{
    return {
        MultiBootPatcher::Id,
        OdinPatcher::Id,
        RamdiskUpdater::Id,
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
    Patcher *p = nullptr;

    if (id == MultiBootPatcher::Id) {
        p = new MultiBootPatcher(this);
    } else if (id == OdinPatcher::Id) {
        p = new OdinPatcher(this);
    } else if (id == RamdiskUpdater::Id) {
        p = new RamdiskUpdater(this);
    }

    if (p != nullptr) {
        m_impl->allocPatchers.push_back(p);
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
    AutoPatcher *ap = nullptr;

    if (id == StandardPatcher::Id) {
        ap = new StandardPatcher(this, info);
    } else if (id == MountCmdPatcher::Id) {
        ap = new MountCmdPatcher(this, info);
    }

    if (ap != nullptr) {
        m_impl->allocAutoPatchers.push_back(ap);
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
    auto it = std::find(m_impl->allocPatchers.begin(),
                        m_impl->allocPatchers.end(),
                        patcher);

    assert(it != m_impl->allocPatchers.end());

    m_impl->allocPatchers.erase(it);
    delete patcher;
}

/*!
 * \brief Destroys an AutoPatcher and frees its memory
 *
 * \param patcher AutoPatcher to destroy
 */
void PatcherConfig::destroyAutoPatcher(AutoPatcher *patcher)
{
    auto it = std::find(m_impl->allocAutoPatchers.begin(),
                        m_impl->allocAutoPatchers.end(),
                        patcher);

    assert(it != m_impl->allocAutoPatchers.end());

    m_impl->allocAutoPatchers.erase(it);
    delete patcher;
}

}
