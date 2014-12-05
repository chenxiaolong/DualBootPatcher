/*
 * Copyright (C) 2014  Xiao-Long Chen <chenxiaolong@cxl.epac.to>
 *
 * This file is part of MultiBootPatcher
 *
 * MultiBootPatcher is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * MultiBootPatcher is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with MultiBootPatcher.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "patcherconfig.h"

#include <boost/algorithm/string/erase.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

#include <libxml/parser.h>
#include <libxml/tree.h>

#include "device.h"
#include "patcherinterface.h"
#include "patchinfo.h"
#include "private/logging.h"
#include "private/regex.h"

// Patchers
#include "patchers/multiboot/multibootpatcher.h"
#include "patchers/primaryupgrade/primaryupgradepatcher.h"
#include "patchers/syncdaemonupdate/syncdaemonupdatepatcher.h"
#include "autopatchers/jflte/jfltepatcher.h"
#include "autopatchers/patchfile/patchfilepatcher.h"
#include "autopatchers/standard/standardpatcher.h"
#include "ramdiskpatchers/bacon/baconramdiskpatcher.h"
#include "ramdiskpatchers/d800/d800ramdiskpatcher.h"
#include "ramdiskpatchers/falcon/falconramdiskpatcher.h"
#include "ramdiskpatchers/flo/floramdiskpatcher.h"
#include "ramdiskpatchers/hammerhead/hammerheadramdiskpatcher.h"
#include "ramdiskpatchers/hlte/hlteramdiskpatcher.h"
#include "ramdiskpatchers/jflte/jflteramdiskpatcher.h"
#include "ramdiskpatchers/klte/klteramdiskpatcher.h"


/*! \cond INTERNAL */
class PatcherConfig::Impl
{
public:
    // Directories
    std::string binariesDir;
    std::string dataDir;
    std::string initsDir;
    std::string patchesDir;
    std::string patchInfosDir;
    std::string scriptsDir;
    std::string tempDir;

    std::string version;
    std::vector<Device *> devices;
    std::vector<std::string> patchinfoIncludeDirs;

    // PatchInfos
    std::vector<PatchInfo *> patchInfos;

    // Partition configurations
    std::vector<PartitionConfig *> partConfigs;

    bool loadedConfig;

    // Errors
    PatcherError error;

    // Created patchers
    std::vector<Patcher *> allocPatchers;
    std::vector<AutoPatcher *> allocAutoPatchers;
    std::vector<RamdiskPatcher *> allocRamdiskPatchers;

    void loadDefaultDevices();
    void loadDefaultPatchers();

    // XML parsing functions for the patchinfo files
    bool loadPatchInfoXml(const std::string &path, const std::string &pathId);
    void parsePatchInfoTagPatchinfo(xmlNode *node, PatchInfo * const info);
    void parsePatchInfoTagMatches(xmlNode *node, PatchInfo * const info);
    void parsePatchInfoTagNotMatched(xmlNode *node, PatchInfo * const info);
    void parsePatchInfoTagName(xmlNode *node, PatchInfo * const info);
    void parsePatchInfoTagRegex(xmlNode *node, PatchInfo * const info);
    void parsePatchInfoTagExcludeRegex(xmlNode *node, PatchInfo * const info);
    void parsePatchInfoTagRegexes(xmlNode *node, PatchInfo * const info);
    void parsePatchInfoTagHasBootImage(xmlNode *node, PatchInfo * const info, const std::string &type);
    void parsePatchInfoTagRamdisk(xmlNode *node, PatchInfo * const info, const std::string &type);
    void parsePatchInfoTagPatchedInit(xmlNode *node, PatchInfo * const info, const std::string &type);
    void parsePatchInfoTagAutopatchers(xmlNode *node, PatchInfo * const info, const std::string &type);
    void parsePatchInfoTagAutopatcher(xmlNode *node, PatchInfo * const info, const std::string &type);
    void parsePatchInfoTagDeviceCheck(xmlNode *node, PatchInfo * const info, const std::string &type);
    void parsePatchInfoTagPartconfigs(xmlNode *node, PatchInfo * const info, const std::string &type);
    void parsePatchInfoTagExclude(xmlNode *node, PatchInfo * const info, const std::string &type);
    void parsePatchInfoTagInclude(xmlNode *node, PatchInfo * const info, const std::string &type);
};
/*! \endcond */


static const std::string BinariesDirName = "binaries";
static const std::string InitsDirName = "inits";
static const std::string PatchesDirName = "patches";
static const std::string PatchInfosDirName = "patchinfos";
static const std::string ScriptsDirName = "scripts";

// --------------------------------

const xmlChar *PatchInfoTagPatchinfo = (xmlChar *) "patchinfo";
const xmlChar *PatchInfoTagMatches = (xmlChar *) "matches";
const xmlChar *PatchInfoTagNotMatched = (xmlChar *) "not-matched";
const xmlChar *PatchInfoTagName = (xmlChar *) "name";
const xmlChar *PatchInfoTagRegex = (xmlChar *) "regex";
const xmlChar *PatchInfoTagExcludeRegex = (xmlChar *) "exclude-regex";
const xmlChar *PatchInfoTagRegexes = (xmlChar *) "regexes";
const xmlChar *PatchInfoTagHasBootImage = (xmlChar *) "has-boot-image";
const xmlChar *PatchInfoTagRamdisk = (xmlChar *) "ramdisk";
const xmlChar *PatchInfoTagPatchedInit = (xmlChar *) "patched-init";
const xmlChar *PatchInfoTagAutopatchers = (xmlChar *) "autopatchers";
const xmlChar *PatchInfoTagAutopatcher = (xmlChar *) "autopatcher";
const xmlChar *PatchInfoTagDeviceCheck = (xmlChar *) "device-check";
const xmlChar *PatchInfoTagPartconfigs = (xmlChar *) "partconfigs";
const xmlChar *PatchInfoTagInclude = (xmlChar *) "include";
const xmlChar *PatchInfoTagExclude = (xmlChar *) "exclude";

const xmlChar *PatchInfoAttrRegex = (xmlChar *) "regex";

const xmlChar *XmlTextTrue = (xmlChar *) "true";
const xmlChar *XmlTextFalse = (xmlChar *) "false";

/*!
 * \class PatcherConfig
 *
 * This is the main interface of the patcher.
 * Blah blah documenting later ;)
 */

PatcherConfig::PatcherConfig() : m_impl(new Impl())
{
    m_impl->loadDefaultDevices();
    m_impl->loadDefaultPatchers();

    m_impl->patchinfoIncludeDirs.push_back("Google_Apps");
    m_impl->patchinfoIncludeDirs.push_back("Other");

    m_impl->version = LIBDBP_VERSION;
}

PatcherConfig::~PatcherConfig()
{
    // Clean up devices
    for (Device *device : m_impl->devices) {
        delete device;
    }
    m_impl->devices.clear();

    // Clean up patchinfos
    for (PatchInfo *info : m_impl->patchInfos) {
        delete info;
    }
    m_impl->patchInfos.clear();

    // Clean up partconfigs
    for (PartitionConfig *config : m_impl->partConfigs) {
        delete config;
    }
    m_impl->partConfigs.clear();

    for (Patcher *patcher : m_impl->allocPatchers) {
        destroyPatcher(patcher);
    }
    m_impl->allocPatchers.clear();

    for (AutoPatcher *patcher : m_impl->allocAutoPatchers) {
        destroyAutoPatcher(patcher);
    }
    m_impl->allocAutoPatchers.clear();

    for (RamdiskPatcher *patcher : m_impl->allocRamdiskPatchers) {
        destroyRamdiskPatcher(patcher);
    }
    m_impl->allocRamdiskPatchers.clear();
}

/*!
 * \brief Get error information
 *
 * \note The returned PatcherError contains valid information only if an
 *       operation has failed.
 *
 * \return PatcherError containing information about the error
 */
PatcherError PatcherConfig::error() const
{
    return m_impl->error;
}

/*!
 * \brief Get binaries directory
 *
 * The detault binaries directory is `<datadir>/binaries/`
 *
 * \return Binaries directory
 */
std::string PatcherConfig::binariesDirectory() const
{
    if (m_impl->binariesDir.empty()) {
        return dataDirectory() + "/" + BinariesDirName;
    } else {
        return m_impl->binariesDir;
    }
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
 * \brief Get init binaries directory
 *
 * The detault binaries directory is `<datadir>/inits/`
 *
 * \return Init binaries directory
 */
std::string PatcherConfig::initsDirectory() const
{
    if (m_impl->initsDir.empty()) {
        return dataDirectory() + "/" + InitsDirName;
    } else {
        return m_impl->initsDir;
    }
}

/*!
 * \brief Get patch files directory
 *
 * The detault binaries directory is `<datadir>/patches/`
 *
 * \return Patch files directory
 */
std::string PatcherConfig::patchesDirectory() const
{
    if (m_impl->patchesDir.empty()) {
        return dataDirectory() + "/" + PatchesDirName;
    } else {
        return m_impl->patchesDir;
    }
}

/*!
 * \brief Get PatchInfo files directory
 *
 * The detault binaries directory is `<datadir>/patchinfos/`
 *
 * \return PatchInfo files directory
 */
std::string PatcherConfig::patchInfosDirectory() const
{
    if (m_impl->patchInfosDir.empty()) {
        return dataDirectory() + "/" + PatchInfosDirName;
    } else {
        return m_impl->patchInfosDir;
    }
}

/*!
 * \brief Get shell scripts directory
 *
 * The detault binaries directory is `<datadir>/scripts/`
 *
 * \return Shell scripts directory
 */
std::string PatcherConfig::scriptsDirectory() const
{
    if (m_impl->scriptsDir.empty()) {
        return dataDirectory() + "/" + ScriptsDirName;
    } else {
        return m_impl->scriptsDir;
    }
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
        return boost::filesystem::temp_directory_path().string();
    } else {
        return m_impl->tempDir;
    }
}

/*!
 * \brief Set binaries directory
 *
 * \note This should only be changed if the default data directory structure is
 *       desired.
 *
 * \param path Path to binaries directory
 */
void PatcherConfig::setBinariesDirectory(std::string path)
{
    m_impl->binariesDir = std::move(path);
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
 * \brief Set init binaries directory
 *
 * \note This should only be changed if the default data directory structure is
 *       not desired.
 *
 * \param path Path to init binaries directory
 */
void PatcherConfig::setInitsDirectory(std::string path)
{
    m_impl->initsDir = std::move(path);
}

/*!
 * \brief Set patch files directory
 *
 * \note This should only be changed if the default data directory structure is
 *       not desired.
 *
 * \param path Path to patch files directory
 */
void PatcherConfig::setPatchesDirectory(std::string path)
{
    m_impl->patchesDir = std::move(path);
}

/*!
 * \brief Set PatchInfo files directory
 *
 * \note This should only be changed if the default data directory structure is
 *       not desired.
 *
 * \param path Path to PatchInfo files directory
 */
void PatcherConfig::setPatchInfosDirectory(std::string path)
{
    m_impl->patchInfosDir = std::move(path);
}

/*!
 * \brief Set shell scripts directory
 *
 * \note This should only be changed if the default data directory structure is
 *       not desired.
 *
 * \param path Path to shell scripts directory
 */
void PatcherConfig::setScriptsDirectory(std::string path)
{
    m_impl->scriptsDir = std::move(path);
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
 * \brief Get version number of the patcher
 *
 * \return Version number
 */
std::string PatcherConfig::version() const
{
    return m_impl->version;
}

/*!
 * \brief Get list of supported devices
 *
 * \return List of supported devices
 */
std::vector<Device *> PatcherConfig::devices() const
{
    return m_impl->devices;
}

/*!
 * \brief Get list of PatchInfos
 *
 * \return List of PatchInfos
 */
std::vector<PatchInfo *> PatcherConfig::patchInfos() const
{
    return m_impl->patchInfos;
}

/*!
 * \brief Get list of PatchInfos for a device
 *
 * \param device Supported device
 *
 * \return List of PatchInfos
 */
std::vector<PatchInfo *> PatcherConfig::patchInfos(const Device * const device) const
{
    std::vector<PatchInfo *> l;

    for (PatchInfo *info : m_impl->patchInfos) {
        if (boost::starts_with(info->id(), device->codename())) {
            l.push_back(info);
            continue;
        }

        for (auto const &include : m_impl->patchinfoIncludeDirs) {
            if (boost::starts_with(info->id(), include)) {
                l.push_back(info);
                break;
            }
        }
    }

    return l;
}

/*!
 * \brief Find matching PatchInfo for a file
 *
 * \param device Supported device
 * \param filename Supported file
 *
 * \return PatchInfo if found, otherwise nullptr
 */
PatchInfo * PatcherConfig::findMatchingPatchInfo(Device *device,
                                                 const std::string &filename) const
{
    if (device == nullptr) {
        return nullptr;
    }

    if (filename.empty()) {
        return nullptr;
    }

    std::string noPath = boost::filesystem::path(filename).filename().string();

    for (PatchInfo *info : patchInfos(device)) {
        for (auto const &regex : info->regexes()) {
            if (MBP_regex_search(noPath, MBP_regex(regex))) {
                bool skipCurInfo = false;

                // If the regex matches, make sure the filename isn't matched
                // by one of the exclusion regexes
                for (auto const &excludeRegex : info->excludeRegexes()) {
                    if (MBP_regex_search(noPath, MBP_regex(excludeRegex))) {
                        skipCurInfo = true;
                        break;
                    }
                }

                if (skipCurInfo) {
                    break;
                }

                return info;
            }
        }
    }

    return nullptr;
}

void PatcherConfig::Impl::loadDefaultDevices()
{
    Device *device;

    // Samsung Galaxy S 4
    device = new Device();
    device->setCodename("jflte");
    device->setName("Samsung Galaxy S 4");
    device->setSelinux(Device::SelinuxPermissive);
    device->setPartition(Device::SystemPartition, "mmcblk0p16");
    device->setPartition(Device::CachePartition, "mmcblk0p18");
    device->setPartition(Device::DataPartition, "mmcblk0p29");
    devices.push_back(device);

    // Samsung Galaxy S 5
    device = new Device();
    device->setCodename("klte");
    device->setName("Samsung Galaxy S 5");
    device->setSelinux(Device::SelinuxPermissive);
    device->setPartition(Device::SystemPartition, "mmcblk0p23");
    device->setPartition(Device::CachePartition, "mmcblk0p24");
    device->setPartition(Device::DataPartition, "mmcblk0p26");
    devices.push_back(device);

    // Samsung Galaxy Note 3
    device = new Device();
    device->setCodename("hlte");
    device->setName("Samsung Galaxy Note 3");
    device->setSelinux(Device::SelinuxPermissive);
    device->setPartition(Device::SystemPartition, "mmcblk0p23");
    device->setPartition(Device::CachePartition, "mmcblk0p24");
    device->setPartition(Device::DataPartition, "mmcblk0p26");
    devices.push_back(device);

    // Google/LG Nexus 5
    device = new Device();
    device->setCodename("hammerhead");
    device->setName("Google/LG Nexus 5");
    device->setSelinux(Device::SelinuxUnchanged);
    devices.push_back(device);

    // Google/ASUS Nexus 7 (2013)
    device = new Device();
    device->setCodename("flo");
    device->setName("Google/ASUS Nexus 7 (2013)");
    device->setSelinux(Device::SelinuxUnchanged);
    devices.push_back(device);

    // OnePlus One
    device = new Device();
    device->setCodename("bacon");
    device->setName("OnePlus One");
    device->setSelinux(Device::SelinuxUnchanged);
    devices.push_back(device);

    // LG G2
    device = new Device();
    device->setCodename("d800");
    device->setName("LG G2");
    device->setSelinux(Device::SelinuxUnchanged);
    devices.push_back(device);

    // Falcon
    device = new Device();
    device->setCodename("falcon");
    device->setName("Motorola Moto G");
    device->setSelinux(Device::SelinuxUnchanged);
    devices.push_back(device);
}

void PatcherConfig::Impl::loadDefaultPatchers()
{
    auto configs1 = MultiBootPatcher::partConfigs();
    auto configs2 = PrimaryUpgradePatcher::partConfigs();

    partConfigs.insert(partConfigs.end(), configs1.begin(), configs1.end());
    partConfigs.insert(partConfigs.end(), configs2.begin(), configs2.end());
}

/*!
 * \brief Get list of Patcher IDs
 *
 * \return List of Patcher names
 */
std::vector<std::string> PatcherConfig::patchers() const
{
    std::vector<std::string> list;
    list.push_back(MultiBootPatcher::Id);
    list.push_back(PrimaryUpgradePatcher::Id);
    list.push_back(SyncdaemonUpdatePatcher::Id);
    return list;
}

/*!
 * \brief Get list of AutoPatcher IDs
 *
 * \return List of AutoPatcher names
 */
std::vector<std::string> PatcherConfig::autoPatchers() const
{
    std::vector<std::string> list;
    list.push_back(JflteDalvikCachePatcher::Id);
    list.push_back(JflteGoogleEditionPatcher::Id);
    list.push_back(JflteSlimAromaBundledMount::Id);
    list.push_back(JflteImperiumPatcher::Id);
    list.push_back(JflteNegaliteNoWipeData::Id);
    list.push_back(JflteTriForceFixAroma::Id);
    list.push_back(JflteTriForceFixUpdate::Id);
    list.push_back(PatchFilePatcher::Id);
    list.push_back(StandardPatcher::Id);
    return list;
}

/*!
 * \brief Get list of RamdiskPatcher IDs
 *
 * \return List of RamdiskPatcher names
 */
std::vector<std::string> PatcherConfig::ramdiskPatchers() const
{
    std::vector<std::string> list;
    list.push_back(BaconRamdiskPatcher::Id);
    list.push_back(D800RamdiskPatcher::Id);
    list.push_back(FalconRamdiskPatcher::Id);
    list.push_back(FloAOSPRamdiskPatcher::Id);
    list.push_back(HammerheadAOSPRamdiskPatcher::Id);
    list.push_back(HlteAOSPRamdiskPatcher::Id);
    list.push_back(JflteAOSPRamdiskPatcher::Id);
    list.push_back(JflteGoogleEditionRamdiskPatcher::Id);
    list.push_back(JflteTouchWizRamdiskPatcher::Id);
    list.push_back(KlteAOSPRamdiskPatcher::Id);
    list.push_back(KlteTouchWizRamdiskPatcher::Id);
    return list;
}

/*!
 * \brief Get Patcher's friendly name
 *
 * \param id Patcher ID
 *
 * \return Patcher's name
 */
std::string PatcherConfig::patcherName(const std::string &id) const
{
    if (id == MultiBootPatcher::Id) {
        return MultiBootPatcher::Name;
    } else if (id == PrimaryUpgradePatcher::Id) {
        return PrimaryUpgradePatcher::Name;
    } else if (id == SyncdaemonUpdatePatcher::Id) {
        return SyncdaemonUpdatePatcher::Name;
    }

    return std::string();
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
    } else if (id == PrimaryUpgradePatcher::Id) {
        p = new PrimaryUpgradePatcher(this);
    } else if (id == SyncdaemonUpdatePatcher::Id) {
        p = new SyncdaemonUpdatePatcher(this);
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
 * \param args AutoPatcher arguments
 *
 * \return New AutoPatcher
 */
AutoPatcher * PatcherConfig::createAutoPatcher(const std::string &id,
                                               const FileInfo * const info,
                                               const PatchInfo::AutoPatcherArgs &args)
{
    AutoPatcher *ap = nullptr;

    if (id == JflteDalvikCachePatcher::Id) {
        ap = new JflteDalvikCachePatcher(this, info);
    } else if (id == JflteGoogleEditionPatcher::Id) {
        ap = new JflteGoogleEditionPatcher(this, info);
    } else if (id == JflteSlimAromaBundledMount::Id) {
        ap = new JflteSlimAromaBundledMount(this, info);
    } else if (id == JflteImperiumPatcher::Id) {
        ap = new JflteImperiumPatcher(this, info);
    } else if (id == JflteNegaliteNoWipeData::Id) {
        ap = new JflteNegaliteNoWipeData(this, info);
    } else if (id == JflteTriForceFixAroma::Id) {
        ap = new JflteTriForceFixAroma(this, info);
    } else if (id == JflteTriForceFixUpdate::Id) {
        ap = new JflteTriForceFixUpdate(this, info);
    } else if (id == PatchFilePatcher::Id) {
        ap = new PatchFilePatcher(this, info, args);
    } else if (id == StandardPatcher::Id) {
        ap = new StandardPatcher(this, info, args);
    }

    if (ap != nullptr) {
        m_impl->allocAutoPatchers.push_back(ap);
    }

    return ap;
}

/*!
 * \brief Create new RamdiskPatcher
 *
 * \param id RamdiskPatcher ID
 * \param info FileInfo describing file to be patched
 * \param cpio CpioFile for the ramdisk archive
 *
 * \return New RamdiskPatcher
 */
RamdiskPatcher * PatcherConfig::createRamdiskPatcher(const std::string &id,
                                                     const FileInfo * const info,
                                                     CpioFile * const cpio)
{
    RamdiskPatcher *rp = nullptr;

    if (id == BaconRamdiskPatcher::Id) {
        rp = new BaconRamdiskPatcher(this, info, cpio);
    } else if (id == D800RamdiskPatcher::Id) {
        rp = new D800RamdiskPatcher(this, info, cpio);
    } else if (id == FalconRamdiskPatcher::Id) {
        rp = new FalconRamdiskPatcher(this, info, cpio);
    } else if (id == FloAOSPRamdiskPatcher::Id) {
        rp = new FloAOSPRamdiskPatcher(this, info, cpio);
    } else if (id == HammerheadAOSPRamdiskPatcher::Id) {
        rp = new HammerheadAOSPRamdiskPatcher(this, info, cpio);
    } else if (id == HlteAOSPRamdiskPatcher::Id) {
        rp = new HlteAOSPRamdiskPatcher(this, info, cpio);
    } else if (id == HlteTouchWizRamdiskPatcher::Id) {
        rp = new HlteTouchWizRamdiskPatcher(this, info, cpio);
    } else if (id == JflteAOSPRamdiskPatcher::Id) {
        rp = new JflteAOSPRamdiskPatcher(this, info, cpio);
    } else if (id == JflteGoogleEditionRamdiskPatcher::Id) {
        rp = new JflteGoogleEditionRamdiskPatcher(this, info, cpio);
    } else if (id == JflteTouchWizRamdiskPatcher::Id) {
        rp = new JflteTouchWizRamdiskPatcher(this, info, cpio);
    } else if (id == KlteAOSPRamdiskPatcher::Id) {
        rp = new KlteAOSPRamdiskPatcher(this, info, cpio);
    } else if (id == KlteTouchWizRamdiskPatcher::Id) {
        rp = new KlteTouchWizRamdiskPatcher(this, info, cpio);
    }

    if (rp != nullptr) {
        m_impl->allocRamdiskPatchers.push_back(rp);
    }

    return rp;
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
}

/*!
 * \brief Destroys a RamdiskPatcher and frees its memory
 *
 * \param patcher RamdiskPatcher to destroy
 */
void PatcherConfig::destroyRamdiskPatcher(RamdiskPatcher *patcher)
{
    auto it = std::find(m_impl->allocRamdiskPatchers.begin(),
                        m_impl->allocRamdiskPatchers.end(),
                        patcher);

    assert(it != m_impl->allocRamdiskPatchers.end());

    m_impl->allocRamdiskPatchers.erase(it);
}

/*!
 * \brief Get list of partition configurations
 *
 * \return List of partition configurations
 */
std::vector<PartitionConfig *> PatcherConfig::partitionConfigs() const
{
    return m_impl->partConfigs;
}

// Based on the code from:
// [1] https://svn.boost.org/trac/boost/ticket/1976
// [2] https://svn.boost.org/trac/boost/ticket/6249
// [3] https://svn.boost.org/trac/boost/attachment/ticket/6249/make_relative_append_example.cpp
static boost::filesystem::path makeRelative(boost::filesystem::path from,
                                            boost::filesystem::path to)
{
    from = boost::filesystem::absolute(from);
    to = boost::filesystem::absolute(to);
    boost::filesystem::path ret;
    boost::filesystem::path::const_iterator iterFrom(from.begin());
    boost::filesystem::path::const_iterator iterTo(to.begin());

    // Find common base
    for (boost::filesystem::path::const_iterator toEnd(to.end()), fromEnd(from.end());
            iterFrom != fromEnd && iterTo != toEnd && *iterFrom == *iterTo;
            ++iterFrom, ++iterTo);

    // Navigate backwards in directory to reach previously found base
    for (boost::filesystem::path::const_iterator fromEnd(from.end());
            iterFrom != fromEnd; ++iterFrom) {
        if (*iterFrom != ".") {
            ret /= "..";
        }
    }

    // Now navigate down the directory branch
    for (; iterTo != to.end(); ++iterTo) {
        ret /= *iterTo;
    }
    return ret;
}

/*!
 * \brief Get list of init binaries
 *
 * \return List of init binaries
 */
std::vector<std::string> PatcherConfig::initBinaries() const
{
    std::vector<std::string> inits;

    try {
        const boost::filesystem::path dirPath(initsDirectory());

        boost::filesystem::recursive_directory_iterator it(dirPath);
        boost::filesystem::recursive_directory_iterator end;

        for (; it != end; ++it) {
            if (boost::filesystem::is_regular_file(it->status())) {
                boost::filesystem::path relPath = makeRelative(dirPath, it->path());
                inits.push_back(relPath.string());
            }
        }
    } catch (std::exception &e) {
        Log::log(Log::Warning, e.what());
    }

    std::sort(inits.begin(), inits.end());

    return inits;
}

/*!
 * \brief Load all PatchInfo XML files
 *
 * \return Whether or not the XML files were successfully read
 */
bool PatcherConfig::loadPatchInfos()
{
    try {
        const boost::filesystem::path dirPath(patchInfosDirectory());

        boost::filesystem::recursive_directory_iterator it(dirPath);
        boost::filesystem::recursive_directory_iterator end;

        for (; it != end; ++it) {
            if (boost::filesystem::is_regular_file(it->status())
                    && it->path().extension() == ".xml") {
                boost::filesystem::path relPath = makeRelative(dirPath, it->path());
                std::string id = relPath.string();
                boost::erase_tail(id, 4);

                if (!m_impl->loadPatchInfoXml(it->path().string(), id)) {
                    m_impl->error = PatcherError::createXmlError(
                            MBP::ErrorCode::XmlParseFileError,
                            it->path().string());
                    return false;
                }
            }
        }

        return true;
    } catch (std::exception &e) {
        Log::log(Log::Warning, e.what());
    }

    return false;
}

bool PatcherConfig::Impl::loadPatchInfoXml(const std::string &path,
                                           const std::string &pathId)
{
    (void) pathId;

    LIBXML_TEST_VERSION

    xmlDoc *doc = xmlReadFile(path.c_str(), nullptr, 0);
    if (doc == nullptr) {
        error = PatcherError::createXmlError(
                MBP::ErrorCode::XmlParseFileError, path);
        return false;
    }

    xmlNode *root = xmlDocGetRootElement(doc);

    for (auto *curNode = root; curNode; curNode = curNode->next) {
        if (curNode->type != XML_ELEMENT_NODE) {
            continue;
        }

        if (xmlStrcmp(curNode->name, PatchInfoTagPatchinfo) == 0) {
            PatchInfo *info = new PatchInfo();
            parsePatchInfoTagPatchinfo(curNode, info);
            info->setId(pathId);
            patchInfos.push_back(info);
        } else {
            Log::log(Log::Warning, "Unknown tag: %s", (char *) curNode->name);
        }
    }

    xmlFreeDoc(doc);
    xmlCleanupParser();

    return true;
}

static std::string xmlStringToStdString(const xmlChar* xmlString) {
    if (xmlString) {
        return std::string(reinterpret_cast<const char*>(xmlString));
    } else {
        return std::string();
    }
}

void PatcherConfig::Impl::parsePatchInfoTagPatchinfo(xmlNode *node,
                                                     PatchInfo * const info)
{
    assert(xmlStrcmp(node->name, PatchInfoTagPatchinfo) == 0);

    for (auto *curNode = node->children; curNode; curNode = curNode->next) {
        if (curNode->type != XML_ELEMENT_NODE) {
            continue;
        }

        if (xmlStrcmp(curNode->name, PatchInfoTagPatchinfo) == 0) {
            Log::log(Log::Warning, "Nested <patchinfo> is not allowed");
        } else if (xmlStrcmp(curNode->name, PatchInfoTagMatches) == 0) {
            parsePatchInfoTagMatches(curNode, info);
        } else if (xmlStrcmp(curNode->name, PatchInfoTagNotMatched) == 0) {
            parsePatchInfoTagNotMatched(curNode, info);
        } else if (xmlStrcmp(curNode->name, PatchInfoTagName) == 0) {
            parsePatchInfoTagName(curNode, info);
        } else if (xmlStrcmp(curNode->name, PatchInfoTagRegex) == 0) {
            parsePatchInfoTagRegex(curNode, info);
        } else if (xmlStrcmp(curNode->name, PatchInfoTagRegexes) == 0) {
            parsePatchInfoTagRegexes(curNode, info);
        } else if (xmlStrcmp(curNode->name, PatchInfoTagHasBootImage) == 0) {
            parsePatchInfoTagHasBootImage(curNode, info, PatchInfo::Default);
        } else if (xmlStrcmp(curNode->name, PatchInfoTagRamdisk) == 0) {
            parsePatchInfoTagRamdisk(curNode, info, PatchInfo::Default);
        } else if (xmlStrcmp(curNode->name, PatchInfoTagPatchedInit) == 0) {
            parsePatchInfoTagPatchedInit(curNode, info, PatchInfo::Default);
        } else if (xmlStrcmp(curNode->name, PatchInfoTagAutopatchers) == 0) {
            parsePatchInfoTagAutopatchers(curNode, info, PatchInfo::Default);
        } else if (xmlStrcmp(curNode->name, PatchInfoTagDeviceCheck) == 0) {
            parsePatchInfoTagDeviceCheck(curNode, info, PatchInfo::Default);
        } else if (xmlStrcmp(curNode->name, PatchInfoTagPartconfigs) == 0) {
            parsePatchInfoTagPartconfigs(curNode, info, PatchInfo::Default);
        } else {
            Log::log(Log::Warning, "Unrecognized tag within <patchinfo>: %s",
                     (char *) curNode->name);
        }
    }
}

void PatcherConfig::Impl::parsePatchInfoTagMatches(xmlNode *node,
                                                   PatchInfo * const info)
{
    assert(xmlStrcmp(node->name, PatchInfoTagMatches) == 0);

    xmlChar *value = xmlGetProp(node, PatchInfoAttrRegex);
    if (value == nullptr) {
        Log::log(Log::Warning, "<matches> element has no 'regex' attribute");
        return;
    }

    const std::string regex = xmlStringToStdString(value);
    xmlFree(value);

    auto regexes = info->condRegexes();
    regexes.push_back(regex);
    info->setCondRegexes(std::move(regexes));

    for (auto *curNode = node->children; curNode; curNode = curNode->next) {
        if (curNode->type != XML_ELEMENT_NODE) {
            continue;
        }

        if (xmlStrcmp(curNode->name, PatchInfoTagMatches) == 0) {
            Log::log(Log::Warning, "Nested <matches> is not allowed");
        } else if (xmlStrcmp(curNode->name, PatchInfoTagHasBootImage) == 0) {
            parsePatchInfoTagHasBootImage(curNode, info, regex);
        } else if (xmlStrcmp(curNode->name, PatchInfoTagRamdisk) == 0) {
            parsePatchInfoTagRamdisk(curNode, info, regex);
        } else if (xmlStrcmp(curNode->name, PatchInfoTagPatchedInit) == 0) {
            parsePatchInfoTagPatchedInit(curNode, info, regex);
        } else if (xmlStrcmp(curNode->name, PatchInfoTagAutopatchers) == 0) {
            parsePatchInfoTagAutopatchers(curNode, info, regex);
        } else if (xmlStrcmp(curNode->name, PatchInfoTagDeviceCheck) == 0) {
            parsePatchInfoTagDeviceCheck(curNode, info, regex);
        } else if (xmlStrcmp(curNode->name, PatchInfoTagPartconfigs) == 0) {
            parsePatchInfoTagPartconfigs(curNode, info, regex);
        } else {
            Log::log(Log::Warning, "Unrecognized tag within <matches>: %s",
                     (char *) curNode->name);
        }
    }
}

void PatcherConfig::Impl::parsePatchInfoTagNotMatched(xmlNode *node,
                                                      PatchInfo * const info)
{
    assert(xmlStrcmp(node->name, PatchInfoTagNotMatched) == 0);

    info->setHasNotMatched(true);

    for (auto *curNode = node->children; curNode; curNode = curNode->next) {
        if (curNode->type != XML_ELEMENT_NODE) {
            continue;
        }

        if (xmlStrcmp(curNode->name, PatchInfoTagNotMatched) == 0) {
            Log::log(Log::Warning, "Nested <not-matched> is not allowed");
        } else if (xmlStrcmp(curNode->name, PatchInfoTagHasBootImage) == 0) {
            parsePatchInfoTagHasBootImage(curNode, info, PatchInfo::NotMatched);
        } else if (xmlStrcmp(curNode->name, PatchInfoTagRamdisk) == 0) {
            parsePatchInfoTagRamdisk(curNode, info, PatchInfo::NotMatched);
        } else if (xmlStrcmp(curNode->name, PatchInfoTagPatchedInit) == 0) {
            parsePatchInfoTagPatchedInit(curNode, info, PatchInfo::NotMatched);
        } else if (xmlStrcmp(curNode->name, PatchInfoTagAutopatchers) == 0) {
            parsePatchInfoTagAutopatchers(curNode, info, PatchInfo::NotMatched);
        } else if (xmlStrcmp(curNode->name, PatchInfoTagDeviceCheck) == 0) {
            parsePatchInfoTagDeviceCheck(curNode, info, PatchInfo::NotMatched);
        } else if (xmlStrcmp(curNode->name, PatchInfoTagPartconfigs) == 0) {
            parsePatchInfoTagPartconfigs(curNode, info, PatchInfo::NotMatched);
        } else {
            Log::log(Log::Warning, "Unrecognized tag within <not-matched>: %s",
                     (char *) curNode->name);
        }
    }
}

void PatcherConfig::Impl::parsePatchInfoTagName(xmlNode *node,
                                                PatchInfo * const info)
{
    assert(xmlStrcmp(node->name, PatchInfoTagName) == 0);

    bool hasText = false;
    for (auto *curNode = node->children; curNode; curNode = curNode->next) {
        if (curNode->type != XML_TEXT_NODE) {
            continue;
        }

        hasText = true;
        if (info->name().empty()) {
            info->setName(xmlStringToStdString(curNode->content));
        } else {
            Log::log(Log::Warning, "Ignoring additional <name> elements");
        }
    }

    if (!hasText) {
        Log::log(Log::Warning, "<name> tag has no text");
    }
}

void PatcherConfig::Impl::parsePatchInfoTagRegex(xmlNode *node,
                                                 PatchInfo * const info)
{
    assert(xmlStrcmp(node->name, PatchInfoTagRegex) == 0);

    bool hasText = false;
    for (auto *curNode = node->children; curNode; curNode = curNode->next) {
        if (curNode->type != XML_TEXT_NODE) {
            continue;
        }

        hasText = true;
        auto regexes = info->regexes();
        regexes.push_back(xmlStringToStdString(curNode->content));
        info->setRegexes(std::move(regexes));
    }

    if (!hasText) {
        Log::log(Log::Warning, "<regex> tag has no text");
    }
}

void PatcherConfig::Impl::parsePatchInfoTagExcludeRegex(xmlNode *node,
                                                        PatchInfo * const info)
{
    assert(xmlStrcmp(node->name, PatchInfoTagExcludeRegex) == 0);

    bool hasText = false;
    for (auto *curNode = node->children; curNode; curNode = curNode->next) {
        if (curNode->type != XML_TEXT_NODE) {
            continue;
        }

        hasText = true;
        auto regexes = info->excludeRegexes();
        regexes.push_back(xmlStringToStdString(curNode->content));
        info->setExcludeRegexes(std::move(regexes));
    }

    if (!hasText) {
        Log::log(Log::Warning, "<exclude-regex> tag has no text");
    }
}

void PatcherConfig::Impl::parsePatchInfoTagRegexes(xmlNode *node,
                                                   PatchInfo * const info)
{
    assert(xmlStrcmp(node->name, PatchInfoTagRegexes) == 0);

    for (auto *curNode = node->children; curNode; curNode = curNode->next) {
        if (curNode->type != XML_ELEMENT_NODE) {
            continue;
        }

        if (xmlStrcmp(curNode->name, PatchInfoTagRegexes) == 0) {
            Log::log(Log::Warning, "Nested <regexes> is not allowed");
        } else if (xmlStrcmp(curNode->name, PatchInfoTagRegex) == 0) {
            parsePatchInfoTagRegex(curNode, info);
        } else if (xmlStrcmp(curNode->name, PatchInfoTagExcludeRegex) == 0) {
            parsePatchInfoTagExcludeRegex(curNode, info);
        } else {
            Log::log(Log::Warning, "Unrecognized tag within <regexes>: %s",
                     (char *) curNode->name);
        }
    }
}

void PatcherConfig::Impl::parsePatchInfoTagHasBootImage(xmlNode *node,
                                                        PatchInfo * const info,
                                                        const std::string &type)
{
    assert(xmlStrcmp(node->name, PatchInfoTagHasBootImage) == 0);

    bool hasText = false;
    for (auto *curNode = node->children; curNode; curNode = curNode->next) {
        if (curNode->type != XML_TEXT_NODE) {
            continue;
        }

        hasText = true;
        if (xmlStrcmp(curNode->content, XmlTextTrue) == 0) {
            info->setHasBootImage(type, true);
        } else if (xmlStrcmp(curNode->content, XmlTextFalse) == 0) {
            info->setHasBootImage(type, false);
        } else {
            Log::log(Log::Warning, "Unknown value for <has-boot-image>: %s",
                     (char *) curNode->content);
        }
    }

    if (!hasText) {
        Log::log(Log::Warning, "<has-boot-image> tag has no text");
    }
}

void PatcherConfig::Impl::parsePatchInfoTagRamdisk(xmlNode *node,
                                                   PatchInfo * const info,
                                                   const std::string &type)
{
    assert(xmlStrcmp(node->name, PatchInfoTagRamdisk) == 0);

    bool hasText = false;
    for (auto *curNode = node->children; curNode; curNode = curNode->next) {
        if (curNode->type != XML_TEXT_NODE) {
            continue;
        }

        hasText = true;
        if (info->ramdisk(type).empty()) {
            info->setRamdisk(type, xmlStringToStdString(curNode->content));
        } else {
            Log::log(Log::Warning, "Ignoring additional <ramdisk> elements");
        }
    }

    if (!hasText) {
        Log::log(Log::Warning, "<ramdisk> tag has no text");
    }
}

void PatcherConfig::Impl::parsePatchInfoTagPatchedInit(xmlNode *node,
                                                       PatchInfo * const info,
                                                       const std::string &type)
{
    assert(xmlStrcmp(node->name, PatchInfoTagPatchedInit) == 0);

    bool hasText = false;
    for (auto *curNode = node->children; curNode; curNode = curNode->next) {
        if (curNode->type != XML_TEXT_NODE) {
            continue;
        }

        hasText = true;
        if (info->patchedInit(type).empty()) {
            info->setPatchedInit(type, xmlStringToStdString(curNode->content));
        } else {
            Log::log(Log::Warning, "Ignoring additional <patched-init> elements");
        }
    }

    if (!hasText) {
        Log::log(Log::Warning, "<patched-init> tag has no text");
    }
}

void PatcherConfig::Impl::parsePatchInfoTagAutopatchers(xmlNode *node,
                                                        PatchInfo * const info,
                                                        const std::string &type)
{
    assert(xmlStrcmp(node->name, PatchInfoTagAutopatchers) == 0);

    for (auto *curNode = node->children; curNode; curNode = curNode->next) {
        if (curNode->type != XML_ELEMENT_NODE) {
            continue;
        }

        if (xmlStrcmp(curNode->name, PatchInfoTagAutopatchers) == 0) {
            Log::log(Log::Warning, "Nested <autopatchers> is not allowed");
        } else if (xmlStrcmp(curNode->name, PatchInfoTagAutopatcher) == 0) {
            parsePatchInfoTagAutopatcher(curNode, info, type);
        } else {
            Log::log(Log::Warning, "Unrecognized tag within <autopatchers>: %s",
                     (char *) curNode->name);
        }
    }
}

void PatcherConfig::Impl::parsePatchInfoTagAutopatcher(xmlNode *node,
                                                       PatchInfo * const info,
                                                       const std::string &type)
{
    assert(xmlStrcmp(node->name, PatchInfoTagAutopatcher) == 0);

    PatchInfo::AutoPatcherArgs args;

    for (xmlAttr *attr = node->properties; attr; attr = attr->next) {
        auto name = attr->name;
        auto value = xmlGetProp(node, name);

        args[xmlStringToStdString(name)] = xmlStringToStdString(value);

        xmlFree(value);
    }

    bool hasText = false;
    for (auto *curNode = node->children; curNode; curNode = curNode->next) {
        if (curNode->type != XML_TEXT_NODE) {
            continue;
        }

        hasText = true;
        info->addAutoPatcher(type, xmlStringToStdString(curNode->content),
                             std::move(args));
    }

    if (!hasText) {
        Log::log(Log::Warning, "<autopatcher> tag has no text");
    }
}

void PatcherConfig::Impl::parsePatchInfoTagDeviceCheck(xmlNode *node,
                                                       PatchInfo * const info,
                                                       const std::string &type)
{
    assert(xmlStrcmp(node->name, PatchInfoTagDeviceCheck) == 0);

    bool hasText = false;
    for (auto *curNode = node->children; curNode; curNode = curNode->next) {
        if (curNode->type != XML_TEXT_NODE) {
            continue;
        }

        hasText = true;
        if (xmlStrcmp(curNode->content, XmlTextTrue) == 0) {
            info->setDeviceCheck(type, true);
        } else if (xmlStrcmp(curNode->content, XmlTextFalse) == 0) {
            info->setDeviceCheck(type, false);
        } else {
            Log::log(Log::Warning, "Unknown value for <device-check>: %s",
                     (char *) curNode->content);
        }
    }

    if (!hasText) {
        Log::log(Log::Warning, "<device-check> tag has no text");
    }
}

void PatcherConfig::Impl::parsePatchInfoTagPartconfigs(xmlNode *node,
                                                       PatchInfo * const info,
                                                       const std::string &type)
{
    assert(xmlStrcmp(node->name, PatchInfoTagPartconfigs) == 0);

    auto configs = info->supportedConfigs(type);
    if (configs.empty()) {
        configs.push_back("all");
    }
    info->setSupportedConfigs(type, std::move(configs));

    for (auto *curNode = node->children; curNode; curNode = curNode->next) {
        if (curNode->type != XML_ELEMENT_NODE) {
            continue;
        }

        if (xmlStrcmp(curNode->name, PatchInfoTagPartconfigs) == 0) {
            Log::log(Log::Warning, "Nested <partconfigs> is not allowed");
        } else if (xmlStrcmp(curNode->name, PatchInfoTagExclude) == 0) {
            parsePatchInfoTagExclude(curNode, info, type);
        } else if (xmlStrcmp(curNode->name, PatchInfoTagInclude) == 0) {
            parsePatchInfoTagInclude(curNode, info, type);
        } else {
            Log::log(Log::Warning, "Unrecognized tag within <partconfigs>: %s",
                     (char *) curNode->name);
        }
    }
}

void PatcherConfig::Impl::parsePatchInfoTagExclude(xmlNode *node,
                                                   PatchInfo * const info,
                                                   const std::string &type)
{
    assert(xmlStrcmp(node->name, PatchInfoTagExclude) == 0);

    bool hasText = false;
    for (auto *curNode = node->children; curNode; curNode = curNode->next) {
        if (curNode->type != XML_TEXT_NODE) {
            continue;
        }

        hasText = true;

        const std::string text = xmlStringToStdString(curNode->content);
        const std::string negated = "!" + text;

        auto configs = info->supportedConfigs(type);

        auto itYes = std::find(configs.begin(), configs.end(), text);
        if (itYes != configs.end()) {
            configs.erase(itYes);
        }

        auto itNo = std::find(configs.begin(), configs.end(), negated);
        if (itNo != configs.end()) {
            configs.erase(itNo);
        }

        configs.push_back(negated);
        info->setSupportedConfigs(type, std::move(configs));
    }

    if (!hasText) {
        Log::log(Log::Warning, "<exclude> tag has no text");
    }
}

void PatcherConfig::Impl::parsePatchInfoTagInclude(xmlNode *node,
                                                   PatchInfo * const info,
                                                   const std::string &type)
{
    assert(xmlStrcmp(node->name, PatchInfoTagInclude) == 0);

    bool hasText = false;
    for (auto *curNode = node->children; curNode; curNode = curNode->next) {
        if (curNode->type != XML_TEXT_NODE) {
            continue;
        }

        hasText = true;

        const std::string text = xmlStringToStdString(curNode->content);
        const std::string negated = "!" + text;

        auto configs = info->supportedConfigs(type);

        auto itYes = std::find(configs.begin(), configs.end(), text);
        if (itYes != configs.end()) {
            configs.erase(itYes);
        }

        auto itNo = std::find(configs.begin(), configs.end(), negated);
        if (itNo != configs.end()) {
            configs.erase(itNo);
        }

        configs.push_back(text);
        info->setSupportedConfigs(type, std::move(configs));
    }

    if (!hasText) {
        Log::log(Log::Warning, "<include> tag has no text");
    }
}
