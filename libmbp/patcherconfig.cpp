/*
 * Copyright (C) 2015  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include <regex>

#include <boost/algorithm/string/erase.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

#ifndef LIBMBP_MINI
#include <pugixml.hpp>
#endif

#include "device.h"
#ifndef LIBMBP_MINI
#include "patcherinterface.h"
#include "patchinfo.h"
#endif
#include "private/logging.h"

// Patchers
#ifndef LIBMBP_MINI
#include "patchers/mbtoolupdater.h"
#include "patchers/multibootpatcher.h"
#include "autopatchers/patchfilepatcher.h"
#include "autopatchers/standardpatcher.h"
#include "ramdiskpatchers/baconramdiskpatcher.h"
#include "ramdiskpatchers/falconramdiskpatcher.h"
#include "ramdiskpatchers/floramdiskpatcher.h"
#include "ramdiskpatchers/hammerheadramdiskpatcher.h"
#include "ramdiskpatchers/hlteramdiskpatcher.h"
#include "ramdiskpatchers/jflteramdiskpatcher.h"
#include "ramdiskpatchers/klteramdiskpatcher.h"
#include "ramdiskpatchers/lgg2ramdiskpatcher.h"
#include "ramdiskpatchers/lgg3ramdiskpatcher.h"
#include "ramdiskpatchers/trelteramdiskpatcher.h"
#include "ramdiskpatchers/trlteramdiskpatcher.h"
#include "ramdiskpatchers/zerolteramdiskpatcher.h"
#endif


namespace mbp
{

/*! \cond INTERNAL */
class PatcherConfig::Impl
{
public:
    // Directories
    std::string dataDir;
    std::string tempDir;

    std::string version;
    std::vector<Device *> devices;
    std::vector<std::string> patchinfoIncludeDirs;

#ifndef LIBMBP_MINI
    // PatchInfos
    std::vector<PatchInfo *> patchInfos;
#endif

    bool loadedConfig;

    // Errors
    PatcherError error;

#ifndef LIBMBP_MINI
    // Created patchers
    std::vector<Patcher *> allocPatchers;
    std::vector<AutoPatcher *> allocAutoPatchers;
    std::vector<RamdiskPatcher *> allocRamdiskPatchers;
#endif

    void loadDefaultDevices();

#ifndef LIBMBP_MINI
    // XML parsing functions for the patchinfo files
    bool loadPatchInfoXml(const std::string &path, const std::string &pathId);
    void parsePatchInfoTagPatchinfo(pugi::xml_node node, PatchInfo * const info);
    void parsePatchInfoTagName(pugi::xml_node node, PatchInfo * const info);
    void parsePatchInfoTagRegex(pugi::xml_node node, PatchInfo * const info);
    void parsePatchInfoTagExcludeRegex(pugi::xml_node node, PatchInfo * const info);
    void parsePatchInfoTagRegexes(pugi::xml_node node, PatchInfo * const info);
    void parsePatchInfoTagHasBootImage(pugi::xml_node node, PatchInfo * const info);
    void parsePatchInfoTagRamdisk(pugi::xml_node node, PatchInfo * const info);
    void parsePatchInfoTagAutopatchers(pugi::xml_node node, PatchInfo * const info);
    void parsePatchInfoTagAutopatcher(pugi::xml_node node, PatchInfo * const info);
    void parsePatchInfoTagDeviceCheck(pugi::xml_node node, PatchInfo * const info);
#endif
};
/*! \endcond */


#ifndef LIBMBP_MINI
const char *PatchInfoTagPatchinfo = "patchinfo";
const char *PatchInfoTagName = "name";
const char *PatchInfoTagRegex = "regex";
const char *PatchInfoTagExcludeRegex = "exclude-regex";
const char *PatchInfoTagRegexes = "regexes";
const char *PatchInfoTagHasBootImage = "has-boot-image";
const char *PatchInfoTagRamdisk = "ramdisk";
const char *PatchInfoTagAutopatchers = "autopatchers";
const char *PatchInfoTagAutopatcher = "autopatcher";
const char *PatchInfoTagDeviceCheck = "device-check";

const char *PatchInfoAttrRegex = "regex";

const char *XmlTextTrue = "true";
const char *XmlTextFalse = "false";
#endif

/*!
 * \class PatcherConfig
 *
 * This is the main interface of the patcher.
 * Blah blah documenting later ;)
 */

PatcherConfig::PatcherConfig() : m_impl(new Impl())
{
    m_impl->loadDefaultDevices();

    m_impl->patchinfoIncludeDirs.push_back("Google_Apps");
    m_impl->patchinfoIncludeDirs.push_back("Other");

    m_impl->version = LIBMBP_VERSION;
}

PatcherConfig::~PatcherConfig()
{
    // Clean up devices
    for (Device *device : m_impl->devices) {
        delete device;
    }
    m_impl->devices.clear();

#ifndef LIBMBP_MINI
    // Clean up patchinfos
    for (PatchInfo *info : m_impl->patchInfos) {
        delete info;
    }
    m_impl->patchInfos.clear();

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
#endif
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
        return boost::filesystem::temp_directory_path().string();
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

#ifndef LIBMBP_MINI

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
        if (boost::starts_with(info->id(), device->id())) {
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
            if (std::regex_search(noPath, std::regex(regex))) {
                bool skipCurInfo = false;

                // If the regex matches, make sure the filename isn't matched
                // by one of the exclusion regexes
                for (auto const &excludeRegex : info->excludeRegexes()) {
                    if (std::regex_search(noPath, std::regex(excludeRegex))) {
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

#endif

void PatcherConfig::Impl::loadDefaultDevices()
{
    Device *device;

    const std::string qcomBaseDir("/dev/block/platform/msm_sdcc.1/by-name");

    std::string qcomSystem(qcomBaseDir); qcomSystem += "/system";
    std::string qcomCache(qcomBaseDir); qcomCache += "/cache";
    std::string qcomData(qcomBaseDir); qcomData += "/userdata";
    std::string qcomBoot(qcomBaseDir); qcomBoot += "/boot";
    std::string qcomRecovery(qcomBaseDir); qcomRecovery += "/recovery";

    std::string qcomAboot(qcomBaseDir); qcomAboot += "/aboot";
    std::string qcomImgdata(qcomBaseDir); qcomImgdata += "/imgdata";
    std::string qcomMisc(qcomBaseDir); qcomMisc += "/misc";
    std::string qcomModem(qcomBaseDir); qcomModem += "/modem";
    std::string qcomRpm(qcomBaseDir); qcomRpm += "/rpm";
    std::string qcomSbl1(qcomBaseDir); qcomSbl1 += "/sbl1";
    std::string qcomSdi(qcomBaseDir); qcomSdi += "/sdi";
    std::string qcomTz(qcomBaseDir); qcomTz += "/tz";

    // http://forum.xda-developers.com/showpost.php?p=60050072&postcount=5640
    std::string s6EdgeBaseDir("/dev/block/platform/15570000.ufs/by-name");
    std::string s6EdgeSystem(s6EdgeBaseDir); s6EdgeSystem += "/SYSTEM";
    std::string s6EdgeCache(s6EdgeBaseDir); s6EdgeCache += "/CACHE";
    std::string s6EdgeData(s6EdgeBaseDir); s6EdgeData += "/USERDATA";
    std::string s6EdgeBoot(s6EdgeBaseDir); s6EdgeBoot += "/BOOT";
    std::string s6EdgeRecovery(s6EdgeBaseDir); s6EdgeRecovery += "/RECOVERY";
    std::string s6EdgeRadio(s6EdgeBaseDir); s6EdgeRadio += "/RADIO";

    // http://forum.xda-developers.com/showpost.php?p=59801273&postcount=5465
    std::string n4ExynosBaseDir("/dev/block/platform/15540000.dwmmc0/by-name");
    std::string n4ExynosSystem(n4ExynosBaseDir); n4ExynosSystem += "/SYSTEM";
    std::string n4ExynosCache(n4ExynosBaseDir); n4ExynosCache += "/CACHE";
    std::string n4ExynosData(n4ExynosBaseDir); n4ExynosData += "/USERDATA";
    std::string n4ExynosBoot(n4ExynosBaseDir); n4ExynosBoot += "/BOOT";
    std::string n4ExynosRecovery(n4ExynosBaseDir); n4ExynosRecovery += "/RECOVERY";
    std::string n4ExynosRadio(n4ExynosBaseDir); n4ExynosRadio += "/RADIO";
    std::string n4ExynosCdmaRadio(n4ExynosBaseDir); n4ExynosCdmaRadio += "/CDMA-RADIO";

    // Samsung Galaxy S 4
    device = new Device();
    device->setId("jflte");
    device->setCodenames({ "jactivelte", "jflte", "jflteatt", "jfltecan",
                           "jfltecri", "jfltecsp", "jfltespr", "jfltetmo",
                           "jflteusc", "jfltevzw", "jfltexx", "jfltezm",
                           "jftddxx", "jgedlte" });
    device->setName("Samsung Galaxy S 4");
    device->setBlockDevBaseDirs({ qcomBaseDir });
    device->setSystemBlockDevs({ qcomSystem, "/dev/block/mmcblk0p16" });
    device->setCacheBlockDevs({ qcomCache, "/dev/block/mmcblk0p18" });
    device->setDataBlockDevs({ qcomData, "/dev/block/mmcblk0p29" });
    device->setBootBlockDevs({ qcomBoot, "/dev/block/mmcblk0p20" });
    device->setRecoveryBlockDevs({ qcomRecovery });
    device->setExtraBlockDevs({ qcomAboot });
    devices.push_back(device);

    // Samsung Galaxy S 5
    device = new Device();
    device->setId("klte");
    device->setCodenames({ "klte", "kltecan", "kltedv", "kltespr", "klteusc",
                           "kltevzw", "kltexx" });
    device->setName("Samsung Galaxy S 5");
    device->setBlockDevBaseDirs({ qcomBaseDir });
    device->setSystemBlockDevs({ qcomSystem, "/dev/block/mmcblk0p23" });
    device->setCacheBlockDevs({ qcomCache, "/dev/block/mmcblk0p24" });
    device->setDataBlockDevs({ qcomData, "/dev/block/mmcblk0p26" });
    device->setBootBlockDevs({ qcomBoot, "/dev/block/mmcblk0p15" });
    device->setRecoveryBlockDevs({ qcomRecovery });
    devices.push_back(device);

    // Samsung Galaxy S 6 Edge
    device = new Device();
    device->setId("zerolte");
    device->setCodenames({ "zerolte", "zeroltetmo", "zeroltexx" });
    device->setName("Samsung Galaxy S 6 Edge (Untested)");
    device->setBlockDevBaseDirs({ s6EdgeBaseDir });
    device->setSystemBlockDevs({ s6EdgeSystem, "/dev/block/sda15" });
    device->setCacheBlockDevs({ s6EdgeCache, "/dev/block/sda16" });
    device->setDataBlockDevs({ s6EdgeData, "/dev/block/sda17" });
    device->setBootBlockDevs({ s6EdgeBoot, "/dev/block/sda5" });
    device->setRecoveryBlockDevs({ s6EdgeRecovery, "/dev/block/sda6" });
    device->setExtraBlockDevs({ s6EdgeRadio });
    devices.push_back(device);

    // Samsung Galaxy Note 3
    device = new Device();
    device->setId("hlte");
    device->setCodenames({ "hlte", "hltecan", "hltespr", "hltetmo", "hlteusc",
                           "hltevzw", "hltexx" });
    device->setName("Samsung Galaxy Note 3");
    device->setBlockDevBaseDirs({ qcomBaseDir });
    device->setSystemBlockDevs({ qcomSystem, "/dev/block/mmcblk0p23" });
    device->setCacheBlockDevs({ qcomCache, "/dev/block/mmcblk0p24" });
    device->setDataBlockDevs({ qcomData, "/dev/block/mmcblk0p26" });
    device->setBootBlockDevs({ qcomBoot, "/dev/block/mmcblk0p14" });
    device->setRecoveryBlockDevs({ qcomRecovery });
    devices.push_back(device);

    // Samsung Galaxy Note 4 (Snapdragon)
    device = new Device();
    device->setId("trlte");
    device->setCodenames({ "trlte", "trltecan", "trltedt", "trltespr",
                           "trltetmo", "trlteusc", "trltexx" });
    device->setName("Samsung Galaxy Note 4 (Snapdragon)");
    device->setBlockDevBaseDirs({ qcomBaseDir });
    device->setSystemBlockDevs({ qcomSystem, "/dev/block/mmcblk0p24" });
    device->setCacheBlockDevs({ qcomCache, "/dev/block/mmcblk0p25" });
    // Shouldn't be an issue as long as ROMs don't touch the "hidden" partition
    device->setDataBlockDevs({ qcomData, "/dev/block/mmcblk0p26",
                               "/dev/block/mmcblk0p27" });
    device->setBootBlockDevs({ qcomBoot, "/dev/block/mmcblk0p17" });
    device->setRecoveryBlockDevs({ qcomRecovery });
    devices.push_back(device);

    // Samsung Galaxy Note 4 (Exynos)
    device = new Device();
    device->setId("trelte");
    device->setCodenames({ "trelte", "treltektt", "treltelgt", "trelteskt",
                           "treltexx" });
    device->setName("Samsung Galaxy Note 4 (Exynos)");
    device->setBlockDevBaseDirs({ n4ExynosBaseDir });
    device->setSystemBlockDevs({ n4ExynosSystem, "/dev/block/mmcblk0p18" });
    device->setCacheBlockDevs({ n4ExynosCache, "/dev/block/mmcblk0p19" });
    device->setDataBlockDevs({ n4ExynosData, "/dev/block/mmcblk0p21" });
    device->setBootBlockDevs({ n4ExynosBoot, "/dev/block/mmcblk0p9" });
    device->setRecoveryBlockDevs({ n4ExynosRecovery, "/dev/block/mmcblk0p10" });
    device->setExtraBlockDevs({ n4ExynosRadio, n4ExynosCdmaRadio });
    devices.push_back(device);

    // Google/LG Nexus 5
    device = new Device();
    device->setId("hammerhead");
    device->setCodenames({ "hammerhead" });
    device->setName("Google/LG Nexus 5");
    device->setBlockDevBaseDirs({ qcomBaseDir });
    device->setSystemBlockDevs({ qcomSystem, "/dev/block/mmcblk0p25" });
    device->setCacheBlockDevs({ qcomCache, "/dev/block/mmcblk0p27" });
    device->setDataBlockDevs({ qcomData, "/dev/block/mmcblk0p28" });
    device->setBootBlockDevs({ qcomBoot, "/dev/block/mmcblk0p19" });
    device->setExtraBlockDevs({ qcomAboot, qcomImgdata, qcomMisc, qcomModem,
                                qcomRpm, qcomSbl1, qcomSdi, qcomTz });
    device->setRecoveryBlockDevs({ qcomRecovery });
    devices.push_back(device);

    // Google/ASUS Nexus 7 (2013)
    device = new Device();
    device->setId("flo");
    device->setCodenames({ "flo" });
    device->setName("Google/ASUS Nexus 7 (2013)");
    device->setBlockDevBaseDirs({ qcomBaseDir });
    device->setSystemBlockDevs({ qcomSystem, "/dev/block/mmcblk0p22" });
    device->setCacheBlockDevs({ qcomCache, "/dev/block/mmcblk0p23" });
    device->setDataBlockDevs({ qcomData, "/dev/block/mmcblk0p30" });
    device->setBootBlockDevs({ qcomBoot, "/dev/block/mmcblk0p14" });
    device->setRecoveryBlockDevs({ qcomRecovery });
    devices.push_back(device);

    // OnePlus One
    device = new Device();
    device->setId("bacon");
    device->setCodenames({ "bacon" });
    device->setName("OnePlus One");
    device->setBlockDevBaseDirs({ qcomBaseDir });
    device->setSystemBlockDevs({ qcomSystem, "/dev/block/mmcblk0p14" });
    device->setCacheBlockDevs({ qcomCache, "/dev/block/mmcblk0p16" });
    device->setDataBlockDevs({ qcomData, "/dev/block/mmcblk0p28" });
    device->setBootBlockDevs({ qcomBoot, "/dev/block/mmcblk0p7" });
    device->setRecoveryBlockDevs({ qcomRecovery });
    devices.push_back(device);

    // LG G2
    device = new Device();
    device->setId("lgg2");
    device->setCodenames({ "g2", "d800", "d802", "ls980", "vs980" });
    device->setName("LG G2");
    device->setBlockDevBaseDirs({ qcomBaseDir });
    device->setSystemBlockDevs({ qcomSystem, "/dev/block/mmcblk0p34" });
    device->setCacheBlockDevs({ qcomCache, "/dev/block/mmcblk0p35" });
    device->setDataBlockDevs({ qcomData, "/dev/block/mmcblk0p38" });
    device->setBootBlockDevs({ qcomBoot, "/dev/block/mmcblk0p7" });
    device->setRecoveryBlockDevs({ qcomRecovery });
    device->setExtraBlockDevs({ qcomAboot });
    devices.push_back(device);

    // LG G3
    device = new Device();
    device->setId("lgg3");
    device->setCodenames({ "g3", "d850", "d851", "d852", "d855", "f400",
                           "f400k", "ls990", "vs985" });
    device->setName("LG G3");
    device->setBlockDevBaseDirs({ qcomBaseDir });
    device->setSystemBlockDevs({ qcomSystem, "/dev/block/mmcblk0p40" });
    device->setCacheBlockDevs({ qcomCache, "/dev/block/mmcblk0p41" });
    device->setDataBlockDevs({ qcomData, "/dev/block/mmcblk0p43" });
    device->setBootBlockDevs({ qcomBoot, "/dev/block/mmcblk0p18" });
    device->setRecoveryBlockDevs({ qcomRecovery });
    device->setExtraBlockDevs({ qcomAboot });
    devices.push_back(device);

    // Falcon
    device = new Device();
    device->setId("falcon");
    device->setCodenames({ "falcon", "falcon_umts", "falcon_umtsds" });
    device->setName("Motorola Moto G");
    device->setBlockDevBaseDirs({ qcomBaseDir });
    device->setSystemBlockDevs({ qcomSystem /*, TODO */ });
    device->setCacheBlockDevs({ qcomCache /*, TODO */ });
    device->setDataBlockDevs({ qcomData /*, TODO */ });
    device->setBootBlockDevs({ qcomBoot /*, TODO */ });
    device->setRecoveryBlockDevs({ qcomRecovery });
    devices.push_back(device);
}

#ifndef LIBMBP_MINI

/*!
 * \brief Get list of Patcher IDs
 *
 * \return List of Patcher names
 */
std::vector<std::string> PatcherConfig::patchers() const
{
    return {
        MultiBootPatcher::Id,
        MbtoolUpdater::Id
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
        PatchFilePatcher::Id,
        StandardPatcher::Id
    };
}

/*!
 * \brief Get list of RamdiskPatcher IDs
 *
 * \return List of RamdiskPatcher names
 */
std::vector<std::string> PatcherConfig::ramdiskPatchers() const
{
    return {
        BaconRamdiskPatcher::Id,
        FalconRamdiskPatcher::Id,
        FloAOSPRamdiskPatcher::Id,
        HammerheadDefaultRamdiskPatcher::Id,
        HlteDefaultRamdiskPatcher::Id,
        JflteDefaultRamdiskPatcher::Id,
        KlteDefaultRamdiskPatcher::Id,
        LGG2RamdiskPatcher::Id,
        LGG3RamdiskPatcher::Id,
        TrelteDefaultRamdiskPatcher::Id,
        TrlteDefaultRamdiskPatcher::Id,
        ZerolteDefaultRamdiskPatcher::Id
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

    if (id == MbtoolUpdater::Id) {
        p = new MbtoolUpdater(this);
    } else if (id == MultiBootPatcher::Id) {
        p = new MultiBootPatcher(this);
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

    if (id == PatchFilePatcher::Id) {
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
    } else if (id == FalconRamdiskPatcher::Id) {
        rp = new FalconRamdiskPatcher(this, info, cpio);
    } else if (id == FloAOSPRamdiskPatcher::Id) {
        rp = new FloAOSPRamdiskPatcher(this, info, cpio);
    } else if (id == HammerheadDefaultRamdiskPatcher::Id) {
        rp = new HammerheadDefaultRamdiskPatcher(this, info, cpio);
    } else if (id == HlteDefaultRamdiskPatcher::Id) {
        rp = new HlteDefaultRamdiskPatcher(this, info, cpio);
    } else if (id == JflteDefaultRamdiskPatcher::Id) {
        rp = new JflteDefaultRamdiskPatcher(this, info, cpio);
    } else if (id == KlteDefaultRamdiskPatcher::Id) {
        rp = new KlteDefaultRamdiskPatcher(this, info, cpio);
    } else if (id == LGG2RamdiskPatcher::Id) {
        rp = new LGG2RamdiskPatcher(this, info, cpio);
    } else if (id == LGG3RamdiskPatcher::Id) {
        rp = new LGG3RamdiskPatcher(this, info, cpio);
    } else if (id == TrelteDefaultRamdiskPatcher::Id) {
        rp = new TrelteDefaultRamdiskPatcher(this, info, cpio);
    } else if (id == TrlteDefaultRamdiskPatcher::Id) {
        rp = new TrlteDefaultRamdiskPatcher(this, info, cpio);
    } else if (id == ZerolteDefaultRamdiskPatcher::Id) {
        rp = new ZerolteDefaultRamdiskPatcher(this, info, cpio);
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
    delete patcher;
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
 * \brief Load all PatchInfo XML files
 *
 * \return Whether or not the XML files were successfully read
 */
bool PatcherConfig::loadPatchInfos()
{
    try {
        const boost::filesystem::path dirPath(dataDirectory() + "/patchinfos");

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
                            ErrorCode::XmlParseFileError,
                            it->path().string());
                    return false;
                }
            }
        }

        return true;
    } catch (std::exception &e) {
        LOGW(e.what());
    }

    return false;
}

bool PatcherConfig::Impl::loadPatchInfoXml(const std::string &path,
                                           const std::string &pathId)
{
    pugi::xml_document doc;

    auto result = doc.load_file(path.c_str());
    if (!result) {
        error = PatcherError::createXmlError(
                ErrorCode::XmlParseFileError, path);
        return false;
    }

    pugi::xml_node root = doc.root();

    for (pugi::xml_node curNode : root.children()) {
        if (curNode.type() != pugi::xml_node_type::node_element) {
            continue;
        }

        if (strcmp(curNode.name(), PatchInfoTagPatchinfo) == 0) {
            PatchInfo *info = new PatchInfo();
            parsePatchInfoTagPatchinfo(curNode, info);
            info->setId(pathId);
            patchInfos.push_back(info);
        } else {
            FLOGW("Unknown tag: {}", curNode.name());
        }
    }

    return true;
}

void PatcherConfig::Impl::parsePatchInfoTagPatchinfo(pugi::xml_node node,
                                                     PatchInfo * const info)
{
    assert(strcmp(node.name(), PatchInfoTagPatchinfo) == 0);

    for (pugi::xml_node curNode : node.children()) {
        if (curNode.type() != pugi::xml_node_type::node_element) {
            continue;
        }

        if (strcmp(curNode.name(), PatchInfoTagPatchinfo) == 0) {
            LOGW("Nested <patchinfo> is not allowed");
        } else if (strcmp(curNode.name(), PatchInfoTagName) == 0) {
            parsePatchInfoTagName(curNode, info);
        } else if (strcmp(curNode.name(), PatchInfoTagRegex) == 0) {
            parsePatchInfoTagRegex(curNode, info);
        } else if (strcmp(curNode.name(), PatchInfoTagRegexes) == 0) {
            parsePatchInfoTagRegexes(curNode, info);
        } else if (strcmp(curNode.name(), PatchInfoTagHasBootImage) == 0) {
            parsePatchInfoTagHasBootImage(curNode, info);
        } else if (strcmp(curNode.name(), PatchInfoTagRamdisk) == 0) {
            parsePatchInfoTagRamdisk(curNode, info);
        } else if (strcmp(curNode.name(), PatchInfoTagAutopatchers) == 0) {
            parsePatchInfoTagAutopatchers(curNode, info);
        } else if (strcmp(curNode.name(), PatchInfoTagDeviceCheck) == 0) {
            parsePatchInfoTagDeviceCheck(curNode, info);
        } else {
            FLOGW("Unrecognized tag within <patchinfo>: {}", curNode.name());
        }
    }
}

void PatcherConfig::Impl::parsePatchInfoTagName(pugi::xml_node node,
                                                PatchInfo * const info)
{
    assert(strcmp(node.name(), PatchInfoTagName) == 0);

    bool hasText = false;
    for (auto curNode : node.children()) {
        if (curNode.type() != pugi::xml_node_type::node_pcdata) {
            continue;
        }

        hasText = true;
        if (info->name().empty()) {
            info->setName(curNode.value());
        } else {
            LOGW("Ignoring additional <name> elements");
        }
    }

    if (!hasText) {
        LOGW("<name> tag has no text");
    }
}

void PatcherConfig::Impl::parsePatchInfoTagRegex(pugi::xml_node node,
                                                 PatchInfo * const info)
{
    assert(strcmp(node.name(), PatchInfoTagRegex) == 0);

    bool hasText = false;
    for (pugi::xml_node curNode : node.children()) {
        if (curNode.type() != pugi::xml_node_type::node_pcdata) {
            continue;
        }

        hasText = true;
        auto regexes = info->regexes();
        regexes.push_back(curNode.value());
        info->setRegexes(std::move(regexes));
    }

    if (!hasText) {
        LOGW("<regex> tag has no text");
    }
}

void PatcherConfig::Impl::parsePatchInfoTagExcludeRegex(pugi::xml_node node,
                                                        PatchInfo * const info)
{
    assert(strcmp(node.name(), PatchInfoTagExcludeRegex) == 0);

    bool hasText = false;
    for (pugi::xml_node curNode : node.children()) {
        if (curNode.type() != pugi::xml_node_type::node_pcdata) {
            continue;
        }

        hasText = true;
        auto regexes = info->excludeRegexes();
        regexes.push_back(curNode.value());
        info->setExcludeRegexes(std::move(regexes));
    }

    if (!hasText) {
        LOGW("<exclude-regex> tag has no text");
    }
}

void PatcherConfig::Impl::parsePatchInfoTagRegexes(pugi::xml_node node,
                                                   PatchInfo * const info)
{
    assert(strcmp(node.name(), PatchInfoTagRegexes) == 0);

    for (pugi::xml_node curNode : node.children()) {
        if (curNode.type() != pugi::xml_node_type::node_element) {
            continue;
        }

        if (strcmp(curNode.name(), PatchInfoTagRegexes) == 0) {
            LOGW("Nested <regexes> is not allowed");
        } else if (strcmp(curNode.name(), PatchInfoTagRegex) == 0) {
            parsePatchInfoTagRegex(curNode, info);
        } else if (strcmp(curNode.name(), PatchInfoTagExcludeRegex) == 0) {
            parsePatchInfoTagExcludeRegex(curNode, info);
        } else {
            FLOGW("Unrecognized tag within <regexes>: {}", curNode.name());
        }
    }
}

void PatcherConfig::Impl::parsePatchInfoTagHasBootImage(pugi::xml_node node,
                                                        PatchInfo * const info)
{
    assert(strcmp(node.name(), PatchInfoTagHasBootImage) == 0);

    bool hasText = false;
    for (pugi::xml_node curNode : node.children()) {
        if (curNode.type() != pugi::xml_node_type::node_pcdata) {
            continue;
        }

        hasText = true;
        if (strcmp(curNode.value(), XmlTextTrue) == 0) {
            info->setHasBootImage(true);
        } else if (strcmp(curNode.value(), XmlTextFalse) == 0) {
            info->setHasBootImage(false);
        } else {
            FLOGW("Unknown value for <has-boot-image>: {}", curNode.value());
        }
    }

    if (!hasText) {
        LOGW("<has-boot-image> tag has no text");
    }
}

void PatcherConfig::Impl::parsePatchInfoTagRamdisk(pugi::xml_node node,
                                                   PatchInfo * const info)
{
    assert(strcmp(node.name(), PatchInfoTagRamdisk) == 0);

    bool hasText = false;
    for (pugi::xml_node curNode : node.children()) {
        if (curNode.type() != pugi::xml_node_type::node_pcdata) {
            continue;
        }

        hasText = true;
        if (info->ramdisk().empty()) {
            info->setRamdisk(curNode.value());
        } else {
            LOGW("Ignoring additional <ramdisk> elements");
        }
    }

    if (!hasText) {
        LOGW("<ramdisk> tag has no text");
    }
}

void PatcherConfig::Impl::parsePatchInfoTagAutopatchers(pugi::xml_node node,
                                                        PatchInfo * const info)
{
    assert(strcmp(node.name(), PatchInfoTagAutopatchers) == 0);

    for (pugi::xml_node curNode : node.children()) {
        if (curNode.type() != pugi::xml_node_type::node_element) {
            continue;
        }

        if (strcmp(curNode.name(), PatchInfoTagAutopatchers) == 0) {
            LOGW("Nested <autopatchers> is not allowed");
        } else if (strcmp(curNode.name(), PatchInfoTagAutopatcher) == 0) {
            parsePatchInfoTagAutopatcher(curNode, info);
        } else {
            FLOGW("Unrecognized tag within <autopatchers>: {}", curNode.name());
        }
    }
}

void PatcherConfig::Impl::parsePatchInfoTagAutopatcher(pugi::xml_node node,
                                                       PatchInfo * const info)
{
    assert(strcmp(node.name(), PatchInfoTagAutopatcher) == 0);

    PatchInfo::AutoPatcherArgs args;

    for (pugi::xml_attribute attr : node.attributes()) {
        const pugi::char_t *name = attr.name();
        const pugi::char_t *value = attr.value();

        args[name] = value;
    }

    bool hasText = false;
    for (pugi::xml_node curNode : node.children()) {
        if (curNode.type() != pugi::xml_node_type::node_pcdata) {
            continue;
        }

        hasText = true;
        info->addAutoPatcher(curNode.value(), std::move(args));
    }

    if (!hasText) {
        LOGW("<autopatcher> tag has no text");
    }
}

void PatcherConfig::Impl::parsePatchInfoTagDeviceCheck(pugi::xml_node node,
                                                       PatchInfo * const info)
{
    assert(strcmp(node.name(), PatchInfoTagDeviceCheck) == 0);

    bool hasText = false;
    for (pugi::xml_node curNode : node.children()) {
        if (curNode.type() != pugi::xml_node_type::node_pcdata) {
            continue;
        }

        hasText = true;
        if (strcmp(curNode.value(), XmlTextTrue) == 0) {
            info->setDeviceCheck(true);
        } else if (strcmp(curNode.value(), XmlTextFalse) == 0) {
            info->setDeviceCheck(false);
        } else {
            FLOGW("Unknown value for <device-check>: {}", curNode.value());
        }
    }

    if (!hasText) {
        LOGW("<device-check> tag has no text");
    }
}

#endif

}