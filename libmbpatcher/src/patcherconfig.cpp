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
#include "mbpatcher/autopatchers/magiskpatcher.h"
#include "mbpatcher/autopatchers/mountcmdpatcher.h"
#include "mbpatcher/autopatchers/standardpatcher.h"
#include "mbpatcher/patchers/odinpatcher.h"
#include "mbpatcher/patchers/ramdiskupdater.h"
#include "mbpatcher/patchers/zippatcher.h"


namespace mb::patcher
{

/*!
 * \class PatcherConfig
 *
 * This is the main interface of the patcher.
 * Blah blah documenting later ;)
 */

PatcherConfig::PatcherConfig() = default;

PatcherConfig::~PatcherConfig() = default;

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
    return m_error;
}

/*!
 * \brief Get top-level data directory
 *
 * \return Data directory
 */
std::string PatcherConfig::data_directory() const
{
    return m_data_dir;
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
    if (m_temp_dir.empty()) {
        return FileUtils::system_temporary_dir();
    } else {
        return m_temp_dir;
    }
}

/*!
 * \brief Set top-level data directory
 *
 * \param path Path to data directory
 */
void PatcherConfig::set_data_directory(std::string path)
{
    m_data_dir = std::move(path);
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
    m_temp_dir = std::move(path);
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
        MagiskPatcher::Id,
        MountCmdPatcher::Id,
        StandardPatcher::Id,
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
    std::unique_ptr<Patcher> p;

    if (id == OdinPatcher::Id) {
        p = std::make_unique<OdinPatcher>(*this);
    } else if (id == RamdiskUpdater::Id) {
        p = std::make_unique<RamdiskUpdater>(*this);
    } else if (id == ZipPatcher::Id) {
        p = std::make_unique<ZipPatcher>(*this);
    }

    if (!p) {
        return nullptr;
    }

    auto *ptr = p.get();
    m_patchers.push_back(std::move(p));
    return ptr;
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
                                                 const FileInfo &info)
{
    std::unique_ptr<AutoPatcher> ap;

    if (id == MagiskPatcher::Id) {
        ap = std::make_unique<MagiskPatcher>(*this, info);
    } else if (id == MountCmdPatcher::Id) {
        ap = std::make_unique<MountCmdPatcher>(*this, info);
    } else if (id == StandardPatcher::Id) {
        ap = std::make_unique<StandardPatcher>(*this, info);
    }

    if (!ap) {
        return nullptr;
    }

    auto *ptr = ap.get();
    m_auto_patchers.push_back(std::move(ap));
    return ptr;
}

/*!
 * \brief Destroys a Patcher and frees its memory
 *
 * \param patcher Patcher to destroy
 */
void PatcherConfig::destroy_patcher(Patcher *patcher)
{
    auto it = std::find_if(
        m_patchers.begin(),
        m_patchers.end(),
        [&](const std::unique_ptr<Patcher> &item) {
            return item.get() == patcher;
        }
    );
    assert(it != m_patchers.end());
    m_patchers.erase(it);
}

/*!
 * \brief Destroys an AutoPatcher and frees its memory
 *
 * \param patcher AutoPatcher to destroy
 */
void PatcherConfig::destroy_auto_patcher(AutoPatcher *patcher)
{
    auto it = std::find_if(
        m_auto_patchers.begin(),
        m_auto_patchers.end(),
        [&](const std::unique_ptr<AutoPatcher> &item) {
            return item.get() == patcher;
        }
    );
    assert(it != m_auto_patchers.end());
    m_auto_patchers.erase(it);
}

}
