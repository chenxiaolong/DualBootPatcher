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

#include <cassert>

#ifndef LIBMBP_MINI
#include <pugixml.hpp>

#include "libmbpio/path.h"
#endif

#include "device.h"
#ifndef LIBMBP_MINI
#include "patcherinterface.h"
#include "patchinfo.h"
#endif
#include "private/fileutils.h"
#include "private/logging.h"

// Patchers
#ifndef LIBMBP_MINI
#include "patchers/mbtoolupdater.h"
#include "patchers/multibootpatcher.h"
#include "autopatchers/patchfilepatcher.h"
#include "autopatchers/standardpatcher.h"
#include "ramdiskpatchers/bacon.h"
#include "ramdiskpatchers/falcon.h"
#include "ramdiskpatchers/flo.h"
#include "ramdiskpatchers/ghost.h"
#include "ramdiskpatchers/ha3g.h"
#include "ramdiskpatchers/hammerhead.h"
#include "ramdiskpatchers/hllte.h"
#include "ramdiskpatchers/hlte.h"
#include "ramdiskpatchers/jflte.h"
#include "ramdiskpatchers/klimtwifi.h"
#include "ramdiskpatchers/klte.h"
#include "ramdiskpatchers/lgg2.h"
#include "ramdiskpatchers/lgg3.h"
#include "ramdiskpatchers/mondrianwifi.h"
#include "ramdiskpatchers/pepper.h"
#include "ramdiskpatchers/serranods.h"
#include "ramdiskpatchers/trelte.h"
#include "ramdiskpatchers/trlte.h"
#include "ramdiskpatchers/zerolte.h"
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
    bool loadPatchInfosXml(const std::string &path);
    void parsePatchInfoTagPatchinfos(pugi::xml_node node);
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
const char *PatchInfoTagPatchinfos = "patchinfos";
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

const char *PatchInfoAttrId = "id";

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

    m_impl->patchinfoIncludeDirs.push_back("Google_Apps/");
    m_impl->patchinfoIncludeDirs.push_back("Other/");

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
        if (StringUtils::starts_with(info->id(), device->id())) {
            l.push_back(info);
            continue;
        }

        for (auto const &include : m_impl->patchinfoIncludeDirs) {
            if (StringUtils::starts_with(info->id(), include)) {
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

    std::string noPath = io::baseName(filename);

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
    // http://forum.xda-developers.com/showpost.php?p=61876479&postcount=6443
    std::string n4ExynosBaseDir("/dev/block/platform/15540000.dwmmc0/by-name");
    std::string n4ExynosSystem(n4ExynosBaseDir); n4ExynosSystem += "/SYSTEM";
    std::string n4ExynosCache(n4ExynosBaseDir); n4ExynosCache += "/CACHE";
    std::string n4ExynosData(n4ExynosBaseDir); n4ExynosData += "/USERDATA";
    std::string n4ExynosBoot(n4ExynosBaseDir); n4ExynosBoot += "/BOOT";
    std::string n4ExynosRecovery(n4ExynosBaseDir); n4ExynosRecovery += "/RECOVERY";
    std::string n4ExynosRadio(n4ExynosBaseDir); n4ExynosRadio += "/RADIO";
    std::string n4ExynosCdmaRadio(n4ExynosBaseDir); n4ExynosCdmaRadio += "/CDMA-RADIO";

    std::string dwmmcBaseDir("/dev/block/platform/dw_mmc.0/by-name");
    std::string dwmmcSystem(dwmmcBaseDir); dwmmcSystem += "/SYSTEM";
    std::string dwmmcCache(dwmmcBaseDir); dwmmcCache += "/CACHE";
    std::string dwmmcData(dwmmcBaseDir); dwmmcData += "/USERDATA";
    std::string dwmmcBoot(dwmmcBaseDir); dwmmcBoot += "/BOOT";
    std::string dwmmcRecovery(dwmmcBaseDir); dwmmcRecovery += "/RECOVERY";
    std::string dwmmcRadio(dwmmcBaseDir); dwmmcRadio += "/RADIO";
    std::string dwmmcCdmaRadio(dwmmcBaseDir); dwmmcCdmaRadio += "/CDMA-RADIO";

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

    // Samsung Galaxy S 4 Mini Duos
    device = new Device();
    device->setId("serranods");
    device->setCodenames({ "serranods", "serranodsxx" });
    device->setName("Samsung Galaxy S 4 Mini Duos");
    device->setBlockDevBaseDirs({ qcomBaseDir });
    device->setSystemBlockDevs({ qcomSystem, "/dev/block/mmcblk0p21" });
    device->setCacheBlockDevs({ qcomCache, "/dev/block/mmcblk0p22" });
    device->setDataBlockDevs({ qcomData, "/dev/block/mmcblk0p24" });
    device->setBootBlockDevs({ qcomBoot, "/dev/block/mmcblk0p13" });
    device->setRecoveryBlockDevs({ qcomRecovery });
    devices.push_back(device);

    // Samsung Galaxy S 5
    device = new Device();
    device->setId("klte");
    device->setCodenames({ "klte", "kltecan", "kltedv", "kltespr", "kltetmo",
                           "klteusc", "kltevzw", "kltexx" });
    device->setName("Samsung Galaxy S 5");
    device->setBlockDevBaseDirs({ qcomBaseDir });
    device->setSystemBlockDevs({ qcomSystem, "/dev/block/mmcblk0p23" });
    device->setCacheBlockDevs({ qcomCache, "/dev/block/mmcblk0p24" });
    device->setDataBlockDevs({ qcomData, "/dev/block/mmcblk0p26" });
    device->setBootBlockDevs({ qcomBoot, "/dev/block/mmcblk0p15" });
    device->setRecoveryBlockDevs({ qcomRecovery });
    devices.push_back(device);

    // Samsung Galaxy S 6 Reg./Edge
    device = new Device();
    device->setId("zerolte");
    device->setCodenames({
        // Regular variant
        "zeroflte", "zerofltetmo", "zerofltexx",
        // Edge variant
        "zerolte", "zeroltetmo", "zeroltexx"
    });
    device->setName("Samsung Galaxy S 6 Reg./Edge");
    device->setArchitecture("arm64-v8a");
    device->setBlockDevBaseDirs({ s6EdgeBaseDir });
    device->setSystemBlockDevs({ s6EdgeSystem, "/dev/block/sda15" });
    device->setCacheBlockDevs({ s6EdgeCache, "/dev/block/sda16" });
    device->setDataBlockDevs({ s6EdgeData, "/dev/block/sda17" });
    device->setBootBlockDevs({ s6EdgeBoot, "/dev/block/sda5" });
    device->setRecoveryBlockDevs({ s6EdgeRecovery, "/dev/block/sda6" });
    device->setExtraBlockDevs({ s6EdgeRadio });
    devices.push_back(device);

    // Samsung Galaxy S 6 Reg./Edge (Sprint)
    device = new Device();
    device->setId("zeroltespr");
    device->setCodenames({
        // Regular variant
        "zerofltespr",
        // Edge variant
        "zeroltespr"
    });
    device->setName("Samsung Galaxy S 6 Reg./Edge (Sprint)");
    device->setArchitecture("arm64-v8a");
    device->setBlockDevBaseDirs({ s6EdgeBaseDir });
    device->setSystemBlockDevs({ s6EdgeSystem, "/dev/block/sda18" });
    device->setCacheBlockDevs({ s6EdgeCache, "/dev/block/sda19" });
    device->setDataBlockDevs({ s6EdgeData, "/dev/block/sda21" });
    device->setBootBlockDevs({ s6EdgeBoot, "/dev/block/sda8" });
    device->setRecoveryBlockDevs({ s6EdgeRecovery, "/dev/block/sda9" });
    device->setExtraBlockDevs({ s6EdgeRadio });
    devices.push_back(device);

    // Samsung Galaxy Note 3 (Snapdragon)
    device = new Device();
    device->setId("hlte");
    device->setCodenames({ "hlte", "hltecan", "hltespr", "hltetmo", "hlteusc",
                           "hltevzw", "hltexx" });
    device->setName("Samsung Galaxy Note 3 (Snapdragon)");
    device->setBlockDevBaseDirs({ qcomBaseDir });
    device->setSystemBlockDevs({ qcomSystem, "/dev/block/mmcblk0p23" });
    device->setCacheBlockDevs({ qcomCache, "/dev/block/mmcblk0p24" });
    device->setDataBlockDevs({ qcomData, "/dev/block/mmcblk0p26" });
    device->setBootBlockDevs({ qcomBoot, "/dev/block/mmcblk0p14" });
    device->setRecoveryBlockDevs({ qcomRecovery });
    devices.push_back(device);

    // Samsung Galaxy Note 3 (Exynos)
    device = new Device();
    device->setId("ha3g");
    device->setCodenames({ "ha3g" });
    device->setName("Samsung Galaxy Note 3 (Exynos)");
    device->setBlockDevBaseDirs({ dwmmcBaseDir });
    device->setSystemBlockDevs({ dwmmcSystem, "/dev/block/mmcblk0p20" });
    device->setCacheBlockDevs({ dwmmcCache, "/dev/block/mmcblk0p19" });
    device->setDataBlockDevs({ dwmmcData, "/dev/block/mmcblk0p21" });
    device->setBootBlockDevs({ dwmmcBoot, "/dev/block/mmcblk0p9" });
    device->setRecoveryBlockDevs({ dwmmcRecovery, "/dev/block/mmcblk0p10" });
    device->setExtraBlockDevs({ dwmmcRadio, dwmmcCdmaRadio });
    devices.push_back(device);

    // Samsung Galaxy Note 3 Neo
    device = new Device();
    device->setId("hllte");
    device->setCodenames({ "hllte", "hlltexx" });
    device->setName("Samsung Galaxy Note 3 Neo");
    device->setBlockDevBaseDirs({ dwmmcBaseDir });
    device->setSystemBlockDevs({ dwmmcSystem, "/dev/block/mmcblk0p18" });
    device->setCacheBlockDevs({ dwmmcCache, "/dev/block/mmcblk0p19" });
    device->setDataBlockDevs({ dwmmcData, "/dev/block/mmcblk0p21" });
    device->setBootBlockDevs({ dwmmcBoot, "/dev/block/mmcblk0p9" });
    device->setRecoveryBlockDevs({ dwmmcRecovery, "/dev/block/mmcblk0p10" });
    device->setExtraBlockDevs({ dwmmcRadio, dwmmcCdmaRadio });
    devices.push_back(device);

    // Samsung Galaxy Note 4 (Snapdragon)
    device = new Device();
    device->setId("trlte");
    device->setCodenames({ "trlte", "trltecan", "trltedt", "trltespr",
                           "trltetmo", "trlteusc", "trltevzw", "trltexx" });
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

    // Samsung Galaxy Tab S 8.4 (Wifi)
    device = new Device();
    device->setId("klimtwifi");
    device->setCodenames({ "klimtwifi", "klimtwifikx" });
    device->setName("Samsung Galaxy Tab S 8.4 (Wifi)");
    device->setBlockDevBaseDirs({ dwmmcBaseDir });
    device->setSystemBlockDevs({ dwmmcSystem, "/dev/block/mmcblk0p18" });
    device->setCacheBlockDevs({ dwmmcCache, "/dev/block/mmcblk0p19" });
    device->setDataBlockDevs({ dwmmcData, "/dev/block/mmcblk0p21" });
    device->setBootBlockDevs({ dwmmcBoot, "/dev/block/mmcblk0p9" });
    device->setRecoveryBlockDevs({ dwmmcRecovery, "/dev/block/mmcblk0p10" });
    device->setExtraBlockDevs({ dwmmcRadio, dwmmcCdmaRadio });
    devices.push_back(device);

    // Samsung Galaxy Tab Pro 8.4 (Wifi)
    device = new Device();
    device->setId("mondrianwifi");
    device->setCodenames({ "mondrianwifi", "mondrianwifiue", "mondrianwifixx" });
    device->setName("Samsung Galaxy Tab Pro 8.4 (Wifi)");
    device->setBlockDevBaseDirs({ qcomBaseDir });
    device->setSystemBlockDevs({ qcomSystem, "/dev/block/mmcblk0p23" });
    device->setCacheBlockDevs({ qcomCache, "/dev/block/mmcblk0p24" });
    device->setDataBlockDevs({ qcomData, "/dev/block/mmcblk0p26" });
    device->setBootBlockDevs({ qcomBoot, "/dev/block/mmcblk0p14" });
    device->setRecoveryBlockDevs({ qcomRecovery, "/dev/block/mmcblk0p15" });
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
    device->setExtraBlockDevs({ qcomAboot, qcomModem });
    devices.push_back(device);

    // Motorola Moto G (2013)
    device = new Device();
    device->setId("falcon");
    device->setCodenames({ "falcon", "falcon_umts", "falcon_umtsds" });
    device->setName("Motorola Moto G (2013)");
    device->setBlockDevBaseDirs({ qcomBaseDir });
    device->setSystemBlockDevs({ qcomSystem /*, TODO */ });
    device->setCacheBlockDevs({ qcomCache /*, TODO */ });
    device->setDataBlockDevs({ qcomData /*, TODO */ });
    device->setBootBlockDevs({ qcomBoot /*, TODO */ });
    device->setRecoveryBlockDevs({ qcomRecovery });
    devices.push_back(device);

    // Motorola Moto X (2013)
    device = new Device();
    device->setId("ghost");
    device->setCodenames({ "ghost", "ghost_att", "ghost_rcica", "ghost_retail",
                           "ghost_sprint", "ghost_usc", "ghost_verizon",
                           "xt1052", "xt1053", "xt1055", "xt1056", "xt1058",
                           "xt1060", });
    device->setName("Motorola Moto X (2013)");
    device->setBlockDevBaseDirs({ qcomBaseDir });
    device->setSystemBlockDevs({ qcomSystem, "/dev/block/mmcblk0p38" });
    device->setCacheBlockDevs({ qcomCache, "/dev/block/mmcblk0p36" });
    device->setDataBlockDevs({ qcomData, "/dev/block/mmcblk0p40" });
    device->setBootBlockDevs({ qcomBoot, "/dev/block/mmcblk0p33" });
    device->setRecoveryBlockDevs({ qcomRecovery });
    devices.push_back(device);

    // Sony Xperia Sola
    device = new Device();
    device->setId("pepper");
    device->setCodenames({ "pepper", "MT27a", "MT27i" });
    device->setName("Sony Xperia Sola");
    device->setSystemBlockDevs({ "/dev/block/mmcblk0p10" });
    device->setCacheBlockDevs({ "/dev/block/mmcblk0p12" });
    device->setDataBlockDevs({ "/dev/block/mmcblk0p11" });
    device->setBootBlockDevs({ "/dev/block/mmcblk0p9" });
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
        BaconRP::Id,
        FalconRP::Id,
        FloAOSPRP::Id,
        GhostRP::Id,
        Ha3gDefaultRP::Id,
        HammerheadDefaultRP::Id,
        HllteDefaultRP::Id,
        HlteDefaultRP::Id,
        JflteDefaultRP::Id,
        KlimtwifiDefaultRP::Id,
        KlteDefaultRP::Id,
        LGG2RP::Id,
        LGG3RP::Id,
        MondrianwifiDefaultRP::Id,
        PepperDefaultRP::Id,
        SerranodsDefaultRP::Id,
        TrelteDefaultRP::Id,
        TrlteDefaultRP::Id,
        ZerolteDefaultRP::Id
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

    if (id == BaconRP::Id) {
        rp = new BaconRP(this, info, cpio);
    } else if (id == FalconRP::Id) {
        rp = new FalconRP(this, info, cpio);
    } else if (id == FloAOSPRP::Id) {
        rp = new FloAOSPRP(this, info, cpio);
    } else if (id == GhostRP::Id) {
        rp = new GhostRP(this, info, cpio);
    } else if (id == Ha3gDefaultRP::Id) {
        rp = new Ha3gDefaultRP(this, info, cpio);
    } else if (id == HammerheadDefaultRP::Id) {
        rp = new HammerheadDefaultRP(this, info, cpio);
    } else if (id == HllteDefaultRP::Id) {
        rp = new HllteDefaultRP(this, info, cpio);
    } else if (id == HlteDefaultRP::Id) {
        rp = new HlteDefaultRP(this, info, cpio);
    } else if (id == JflteDefaultRP::Id) {
        rp = new JflteDefaultRP(this, info, cpio);
    } else if (id == KlimtwifiDefaultRP::Id) {
        rp = new KlimtwifiDefaultRP(this, info, cpio);
    } else if (id == KlteDefaultRP::Id) {
        rp = new KlteDefaultRP(this, info, cpio);
    } else if (id == LGG2RP::Id) {
        rp = new LGG2RP(this, info, cpio);
    } else if (id == LGG3RP::Id) {
        rp = new LGG3RP(this, info, cpio);
    } else if (id == MondrianwifiDefaultRP::Id) {
        rp = new MondrianwifiDefaultRP(this, info, cpio);
    } else if (id == PepperDefaultRP::Id) {
        rp = new PepperDefaultRP(this, info, cpio);
    } else if (id == SerranodsDefaultRP::Id) {
        rp = new SerranodsDefaultRP(this, info, cpio);
    } else if (id == TrelteDefaultRP::Id) {
        rp = new TrelteDefaultRP(this, info, cpio);
    } else if (id == TrlteDefaultRP::Id) {
        rp = new TrlteDefaultRP(this, info, cpio);
    } else if (id == ZerolteDefaultRP::Id) {
        rp = new ZerolteDefaultRP(this, info, cpio);
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

/*!
 * \brief Load all PatchInfo XML files
 *
 * \return Whether or not the XML files were successfully read
 */
bool PatcherConfig::loadPatchInfos()
{
    std::string path(dataDirectory());
    path += "/patchinfos.gen.xml";

    if (!m_impl->loadPatchInfosXml(path)) {
        m_impl->error = PatcherError::createXmlError(
                ErrorCode::XmlParseFileError, path);
        return false;
    }

    return true;
}

bool PatcherConfig::Impl::loadPatchInfosXml(const std::string &path)
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

        if (strcmp(curNode.name(), PatchInfoTagPatchinfos) == 0) {
            parsePatchInfoTagPatchinfos(curNode);
        } else {
            FLOGW("Unknown root tag: %s", curNode.name());
        }
    }

    return true;
}

void PatcherConfig::Impl::parsePatchInfoTagPatchinfos(pugi::xml_node node)
{
    assert(strcmp(node.name(), PatchInfoTagPatchinfos) == 0);

    for (pugi::xml_node curNode : node.children()) {
        if (curNode.type() != pugi::xml_node_type::node_element) {
            continue;
        }

        if (strcmp(curNode.name(), PatchInfoTagPatchinfos) == 0) {
            LOGW("Nested <patchinfos> is not allowed");
        } else if (strcmp(curNode.name(), PatchInfoTagPatchinfo) == 0) {
            std::string id;
            bool hasId = false;
            for (pugi::xml_attribute attr : curNode.attributes()) {
                if (strcmp(PatchInfoAttrId, attr.name()) == 0) {
                    hasId = true;
                    id = attr.value();
                    break;
                }
            }

            if (!hasId) {
                LOGW("<patchinfo> does not contain the id attribute");
                continue;
            }

            PatchInfo *info = new PatchInfo();
            parsePatchInfoTagPatchinfo(curNode, info);
            info->setId(id);
            patchInfos.push_back(info);
        } else {
            FLOGW("Unrecognized tag within <patchinfos>: %s", curNode.name());
        }
    }
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
            FLOGW("Unrecognized tag within <patchinfo>: %s", curNode.name());
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
            FLOGW("Unrecognized tag within <regexes>: %s", curNode.name());
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
            FLOGW("Unknown value for <has-boot-image>: %s", curNode.value());
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
            FLOGW("Unrecognized tag within <autopatchers>: %s", curNode.name());
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
            FLOGW("Unknown value for <device-check>: %s", curNode.value());
        }
    }

    if (!hasText) {
        LOGW("<device-check> tag has no text");
    }
}

#endif

}
