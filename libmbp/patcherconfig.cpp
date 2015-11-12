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

#include <algorithm>

#include <cassert>

#include "device.h"
#ifndef LIBMBP_MINI
#include "patcherinterface.h"
#endif
#include "private/fileutils.h"
#include "private/logging.h"
#include "version.h"

// Patchers
#ifndef LIBMBP_MINI
#include "patchers/mbtoolupdater.h"
#include "patchers/multibootpatcher.h"
#include "autopatchers/standardpatcher.h"
#include "autopatchers/xposedpatcher.h"
#include "ramdiskpatchers/default.h"
#include "ramdiskpatchers/pepper.h"
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

    // Errors
    ErrorCode error;

#ifndef LIBMBP_MINI
    // Created patchers
    std::vector<Patcher *> allocPatchers;
    std::vector<AutoPatcher *> allocAutoPatchers;
    std::vector<RamdiskPatcher *> allocRamdiskPatchers;
#endif

    void addSamsungDevices();
    void addLenovoDevices();
    void addLgDevices();
    void addMotorolaDevices();
    void addNexusDevices();
    void addOnePlusDevices();
    void addHuaweiDevices();
    void addSonyDevices();
    void loadDefaultDevices();
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
    m_impl->loadDefaultDevices();

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

#define BOOTDEVICE_BASE_DIR     "/dev/block/bootdevice/by-name"
#define BOOTDEVICE_BOOT         BOOTDEVICE_BASE_DIR "/boot"
#define BOOTDEVICE_CACHE        BOOTDEVICE_BASE_DIR "/cache"
#define BOOTDEVICE_RECOVERY     BOOTDEVICE_BASE_DIR "/recovery"
#define BOOTDEVICE_SYSTEM       BOOTDEVICE_BASE_DIR "/system"
#define BOOTDEVICE_USERDATA     BOOTDEVICE_BASE_DIR "/userdata"

#define QCOM_BASE_DIR           "/dev/block/platform/msm_sdcc.1/by-name"
#define QCOM_ABOOT              QCOM_BASE_DIR "/aboot"
#define QCOM_BOOT               QCOM_BASE_DIR "/boot"
#define QCOM_CACHE              QCOM_BASE_DIR "/cache"
#define QCOM_IMGDATA            QCOM_BASE_DIR "/imgdata"
#define QCOM_MISC               QCOM_BASE_DIR "/misc"
#define QCOM_MODEM              QCOM_BASE_DIR "/modem"
#define QCOM_RECOVERY           QCOM_BASE_DIR "/recovery"
#define QCOM_RPM                QCOM_BASE_DIR "/rpm"
#define QCOM_SBL1               QCOM_BASE_DIR "/sbl1"
#define QCOM_SDI                QCOM_BASE_DIR "/sdi"
#define QCOM_SYSTEM             QCOM_BASE_DIR "/system"
#define QCOM_TZ                 QCOM_BASE_DIR "/tz"
#define QCOM_USERDATA           QCOM_BASE_DIR "/userdata"

#define F9824900_BASE_DIR       "/dev/block/platform/f9824900.sdhci/by-name"
#define F9824900_BOOT           F9824900_BASE_DIR "/boot"
#define F9824900_CACHE          F9824900_BASE_DIR "/cache"
#define F9824900_RECOVERY       F9824900_BASE_DIR "/recovery"
#define F9824900_SYSTEM         F9824900_BASE_DIR "/system"
#define F9824900_USERDATA       F9824900_BASE_DIR "/userdata"

// http://forum.xda-developers.com/showpost.php?p=60050072&postcount=5640
#define ZERO_BASE_DIR           "/dev/block/platform/15570000.ufs/by-name"
#define ZERO_BOOT               ZERO_BASE_DIR "/BOOT"
#define ZERO_CACHE              ZERO_BASE_DIR "/CACHE"
#define ZERO_RADIO              ZERO_BASE_DIR "/RADIO"
#define ZERO_RECOVERY           ZERO_BASE_DIR "/RECOVERY"
#define ZERO_SYSTEM             ZERO_BASE_DIR "/SYSTEM"
#define ZERO_USERDATA           ZERO_BASE_DIR "/USERDATA"

// http://forum.xda-developers.com/showpost.php?p=59801273&postcount=5465
// http://forum.xda-developers.com/showpost.php?p=61876479&postcount=6443
#define TRELTE_BASE_DIR         "/dev/block/platform/15540000.dwmmc0/by-name"
#define TRELTE_BOOT             TRELTE_BASE_DIR "/BOOT"
#define TRELTE_CACHE            TRELTE_BASE_DIR "/CACHE"
#define TRELTE_CDMA_RADIO       TRELTE_BASE_DIR "/CDMA-RADIO"
#define TRELTE_RADIO            TRELTE_BASE_DIR "/RADIO"
#define TRELTE_RECOVERY         TRELTE_BASE_DIR "/RECOVERY"
#define TRELTE_SYSTEM           TRELTE_BASE_DIR "/SYSTEM"
#define TRELTE_USERDATA         TRELTE_BASE_DIR "/USERDATA"

#define DWMMC_BASE_DIR          "/dev/block/platform/dw_mmc/by-name"
#define DWMMC_BOOT              DWMMC_BASE_DIR "/BOOT"
#define DWMMC_CACHE             DWMMC_BASE_DIR "/CACHE"
#define DWMMC_RADIO             DWMMC_BASE_DIR "/RADIO"
#define DWMMC_RECOVERY          DWMMC_BASE_DIR "/RECOVERY"
#define DWMMC_SYSTEM            DWMMC_BASE_DIR "/SYSTEM"
#define DWMMC_USERDATA          DWMMC_BASE_DIR "/USERDATA"

#define DWMMC0_BASE_DIR          "/dev/block/platform/dw_mmc.0/by-name"
#define DWMMC0_BOOT              DWMMC0_BASE_DIR "/BOOT"
#define DWMMC0_CACHE             DWMMC0_BASE_DIR "/CACHE"
#define DWMMC0_CDMA_RADIO        DWMMC0_BASE_DIR "/CDMA-RADIO"
#define DWMMC0_RADIO             DWMMC0_BASE_DIR "/RADIO"
#define DWMMC0_RECOVERY          DWMMC0_BASE_DIR "/RECOVERY"
#define DWMMC0_SYSTEM            DWMMC0_BASE_DIR "/SYSTEM"
#define DWMMC0_USERDATA          DWMMC0_BASE_DIR "/USERDATA"

#define MTK_BASE_DIR            "/dev/block/platform/mtk-msdc.0/by-name"
#define MTK_BOOT                MTK_BASE_DIR "/boot"
#define MTK_CACHE               MTK_BASE_DIR "/cache"
#define MTK_RECOVERY            MTK_BASE_DIR "/recovery"
#define MTK_SYSTEM              MTK_BASE_DIR "/system"
#define MTK_USERDATA            MTK_BASE_DIR "/userdata"

#define TEGRA3_BASE_DIR         "/dev/block/platform/sdhci-tegra.3/by-name"
#define TEGRA3_BOOT             TEGRA3_BASE_DIR "/LNX"
#define TEGRA3_CACHE            TEGRA3_BASE_DIR "/CAC"
#define TEGRA3_RECOVERY         TEGRA3_BASE_DIR "/SOS"
#define TEGRA3_SYSTEM           TEGRA3_BASE_DIR "/APP"
#define TEGRA3_USERDATA         TEGRA3_BASE_DIR "/UDA"

#define HISILICON_BASE_DIR      "/dev/block/platform/hi_mci.0/by-name"
#define HISILICON_BOOT          HISILICON_BASE_DIR "/boot"
#define HISILICON_CACHE         HISILICON_BASE_DIR "/cache"
#define HISILICON_RECOVERY      HISILICON_BASE_DIR "/recovery"
#define HISILICON_SYSTEM        HISILICON_BASE_DIR "/system"
#define HISILICON_USERDATA      HISILICON_BASE_DIR "/userdata"

#define INTEL_PCI_BASE_DIR      "/dev/block/pci/pci0000:00/0000:00:01.0/by-name"
#define INTEL_PCI_BOOT          INTEL_PCI_BASE_DIR "/BOOT"
#define INTEL_PCI_CACHE         INTEL_PCI_BASE_DIR "/CACHE"
#define INTEL_PCI_RADIO         INTEL_PCI_BASE_DIR "/RADIO"
#define INTEL_PCI_RECOVERY      INTEL_PCI_BASE_DIR "/RECOVERY"
#define INTEL_PCI_SYSTEM        INTEL_PCI_BASE_DIR "/SYSTEM"
#define INTEL_PCI_USERDATA      INTEL_PCI_BASE_DIR "/USERDATA"

void PatcherConfig::Impl::addSamsungDevices()
{
    Device *device;

    // Samsung Galaxy S 3 (Qcom)
    device = new Device();
    device->setId("d2");
    device->setCodenames({ "d2", "d2lte", "d2att", "d2can", "d2cri", "d2ltetmo",
                           "d2mtr", "d2spi", "d2spr", "d2tfnspr", "d2tfnvzw",
                           "d2tmo", "d2usc", "d2vmu", "d2vzw", "d2xar" });
    device->setName("Samsung Galaxy S 3 (Qcom)");
    device->setBlockDevBaseDirs({ QCOM_BASE_DIR });
    device->setSystemBlockDevs({ QCOM_SYSTEM, "/dev/block/mmcblk0p14" });
    device->setCacheBlockDevs({ QCOM_CACHE, "/dev/block/mmcblk0p17" });
    device->setDataBlockDevs({ QCOM_USERDATA, "/dev/block/mmcblk0p15" });
    device->setBootBlockDevs({ QCOM_BOOT, "/dev/block/mmcblk0p7" });
    device->setRecoveryBlockDevs({ QCOM_RECOVERY, "/dev/block/mmcblk0p18" });
    device->setExtraBlockDevs({ QCOM_ABOOT });
    devices.push_back(device);

    // Samsung Galaxy S 3 (i9300)
    device = new Device();
    device->setId("m0");
    device->setCodenames({ "m0", "i9300", "GT-I9300" });
    device->setName("Samsung Galaxy S 3 (i9300)");
    device->setBlockDevBaseDirs({ DWMMC_BASE_DIR });
    device->setSystemBlockDevs({ DWMMC_SYSTEM, "/dev/block/mmcblk0p9" });
    device->setCacheBlockDevs({ DWMMC_CACHE, "/dev/block/mmcblk0p8" });
    device->setDataBlockDevs({ DWMMC_USERDATA, "/dev/block/mmcblk0p12" });
    device->setBootBlockDevs({ DWMMC_BOOT, "/dev/block/mmcblk0p5" });
    device->setRecoveryBlockDevs({ DWMMC_RECOVERY, "/dev/block/mmcblk0p6" });
    device->setExtraBlockDevs({ DWMMC_RADIO, "/dev/block/mmcblk0p7" });
    devices.push_back(device);

    // Samsung Galaxy S 3 (i9305)
    device = new Device();
    device->setId("m3");
    device->setCodenames({ "m3" });
    device->setName("Samsung Galaxy S 3 (i9305)");
    device->setBlockDevBaseDirs({ DWMMC_BASE_DIR });
    device->setSystemBlockDevs({ DWMMC_SYSTEM, "/dev/block/mmcblk0p13" });
    device->setCacheBlockDevs({ DWMMC_CACHE, "/dev/block/mmcblk0p12" });
    device->setDataBlockDevs({ DWMMC_USERDATA, "/dev/block/mmcblk0p16" });
    device->setBootBlockDevs({ DWMMC_BOOT, "/dev/block/mmcblk0p8" });
    device->setRecoveryBlockDevs({ DWMMC_RECOVERY, "/dev/block/mmcblk0p9" });
    device->setExtraBlockDevs({ DWMMC_RADIO });
    devices.push_back(device);

    // Samsung Galaxy S 4
    device = new Device();
    device->setId("jflte");
    device->setCodenames({
        // Regular variant
        "jflte", "jflteatt", "jfltecan", "jfltecri", "jfltecsp", "jfltespr",
        "jfltetmo", "jflteusc", "jfltevzw", "jfltexx", "jfltezm",
        // Active variant
        "jactivelte",
        // Google Edition variant
        "jgedlte",
        // GT-I9507
        "jftddxx",
        // GT-I9515
        "jfvelte", "jfveltexx"
    });
    device->setName("Samsung Galaxy S 4");
    device->setBlockDevBaseDirs({ QCOM_BASE_DIR });
    device->setSystemBlockDevs({ QCOM_SYSTEM, "/dev/block/mmcblk0p16" });
    device->setCacheBlockDevs({ QCOM_CACHE, "/dev/block/mmcblk0p18" });
    device->setDataBlockDevs({ QCOM_USERDATA, "/dev/block/mmcblk0p29" });
    device->setBootBlockDevs({ QCOM_BOOT, "/dev/block/mmcblk0p20" });
    device->setRecoveryBlockDevs({ QCOM_RECOVERY });
    device->setExtraBlockDevs({ QCOM_ABOOT });
    devices.push_back(device);

    // Samsung Galaxy S 4 LTE-A
    device = new Device();
    device->setId("ks01lte");
    device->setCodenames({ "ks01lte" });
    device->setName("Samsung Galaxy S 4 LTE-A");
    device->setBlockDevBaseDirs({ QCOM_BASE_DIR });
    device->setSystemBlockDevs({ QCOM_SYSTEM, "/dev/block/mmcblk0p23" });
    device->setCacheBlockDevs({ QCOM_CACHE, "/dev/block/mmcblk0p24" });
    device->setDataBlockDevs({ QCOM_USERDATA, "/dev/block/mmcblk0p26" });
    device->setBootBlockDevs({ QCOM_BOOT, "/dev/block/mmcblk0p14" });
    device->setRecoveryBlockDevs({ QCOM_RECOVERY, "/dev/block/mmcblk0p15" });
    devices.push_back(device);

    // Samsung Galaxy S 4 Mini Reg./Duos/LTE
    device = new Device();
    device->setId("serrano");
    device->setCodenames({
        // Regular variant
        "serrano3g", "serrano3gxx",
        // Duos variant
        "serranods", "serranodsxx",
        // LTE variant
        "serranolte", "serranoltexx"
    });
    device->setName("Samsung Galaxy S 4 Mini Reg./Duos/LTE");
    device->setBlockDevBaseDirs({ QCOM_BASE_DIR });
    device->setSystemBlockDevs({ QCOM_SYSTEM, "/dev/block/mmcblk0p21" });
    device->setCacheBlockDevs({ QCOM_CACHE, "/dev/block/mmcblk0p22" });
    device->setDataBlockDevs({ QCOM_USERDATA, "/dev/block/mmcblk0p24" });
    device->setBootBlockDevs({ QCOM_BOOT, "/dev/block/mmcblk0p13" });
    device->setRecoveryBlockDevs({ QCOM_RECOVERY });
    devices.push_back(device);

    // Samsung Galaxy S 5
    device = new Device();
    device->setId("klte");
    device->setCodenames({ "klte", "kltecan", "kltedv", "kltespr", "kltetmo",
                           "klteusc", "kltevzw", "kltexx" });
    device->setName("Samsung Galaxy S 5");
    device->setBlockDevBaseDirs({ QCOM_BASE_DIR });
    device->setSystemBlockDevs({ QCOM_SYSTEM, "/dev/block/mmcblk0p23" });
    device->setCacheBlockDevs({ QCOM_CACHE, "/dev/block/mmcblk0p24" });
    device->setDataBlockDevs({ QCOM_USERDATA, "/dev/block/mmcblk0p26" });
    device->setBootBlockDevs({ QCOM_BOOT, "/dev/block/mmcblk0p15" });
    device->setRecoveryBlockDevs({ QCOM_RECOVERY, "/dev/block/mmcblk0p14" });
    devices.push_back(device);

    // Samsung Galaxy S 5 Broadband LTE-A
    device = new Device();
    device->setId("lentislte");
    device->setCodenames({ "lentislteskt", "lentisltektt", "lentisltelgt",
                           "lentislte" });
    device->setName("Samsung Galaxy S 5 Broadband LTE-A");
    device->setBlockDevBaseDirs({ QCOM_BASE_DIR });
    device->setSystemBlockDevs({ QCOM_SYSTEM, "/dev/block/mmcblk0p24" });
    device->setCacheBlockDevs({ QCOM_CACHE, "/dev/block/mmcblk0p25" });
    device->setDataBlockDevs({ QCOM_USERDATA, "/dev/block/mmcblk0p27" });
    device->setBootBlockDevs({ QCOM_BOOT, "/dev/block/mmcblk0p17" });
    device->setRecoveryBlockDevs({ QCOM_RECOVERY });
    devices.push_back(device);

    // Samsung Galaxy S 6 Flat/Edge
    device = new Device();
    device->setId("zerolte");
    device->setCodenames({
        // Regular variant
        "zeroflte", "zerofltebmc", "zerofltetmo", "zerofltexx",
        // Edge variant
        "zerolte", "zeroltebmc", "zeroltetmo", "zeroltexx"
    });
    device->setName("Samsung Galaxy S 6 Flat/Edge");
    device->setArchitecture(ARCH_ARM64_V8A);
    device->setBlockDevBaseDirs({ ZERO_BASE_DIR });
    device->setSystemBlockDevs({ ZERO_SYSTEM, "/dev/block/sda15" });
    device->setCacheBlockDevs({ ZERO_CACHE, "/dev/block/sda16" });
    device->setDataBlockDevs({ ZERO_USERDATA, "/dev/block/sda17" });
    device->setBootBlockDevs({ ZERO_BOOT, "/dev/block/sda5" });
    device->setRecoveryBlockDevs({ ZERO_RECOVERY, "/dev/block/sda6" });
    device->setExtraBlockDevs({ ZERO_RADIO });
    devices.push_back(device);

    // Samsung Galaxy S 6 Flat/Edge (Sprint)
    device = new Device();
    device->setId("zeroltespr");
    device->setCodenames({
        // Regular variant
        "zerofltespr",
        // Edge variant
        "zeroltespr"
    });
    device->setName("Samsung Galaxy S 6 Flat/Edge (Sprint)");
    device->setArchitecture(ARCH_ARM64_V8A);
    device->setBlockDevBaseDirs({ ZERO_BASE_DIR });
    device->setSystemBlockDevs({ ZERO_SYSTEM, "/dev/block/sda18" });
    device->setCacheBlockDevs({ ZERO_CACHE, "/dev/block/sda19" });
    device->setDataBlockDevs({ ZERO_USERDATA, "/dev/block/sda21" });
    device->setBootBlockDevs({ ZERO_BOOT, "/dev/block/sda8" });
    device->setRecoveryBlockDevs({ ZERO_RECOVERY, "/dev/block/sda9" });
    device->setExtraBlockDevs({ ZERO_RADIO });
    devices.push_back(device);

    // Samsung Galaxy S 6 Edge+
    device = new Device();
    device->setId("zenlte");
    device->setCodenames({ "zenlte", "zenltexx" });
    device->setName("Samsung Galaxy S 6 Edge+");
    device->setArchitecture(ARCH_ARM64_V8A);
    device->setBlockDevBaseDirs({ ZERO_BASE_DIR });
    device->setSystemBlockDevs({ ZERO_SYSTEM, "/dev/block/sda14" });
    device->setCacheBlockDevs({ ZERO_CACHE, "/dev/block/sda15" });
    device->setDataBlockDevs({ ZERO_USERDATA, "/dev/block/sda17" });
    device->setBootBlockDevs({ ZERO_BOOT, "/dev/block/sda5" });
    device->setRecoveryBlockDevs({ ZERO_RECOVERY, "/dev/block/sda6" });
    device->setExtraBlockDevs({ ZERO_RADIO });
    devices.push_back(device);

    // Samsung Galaxy Note 3 (Snapdragon)
    device = new Device();
    device->setId("hlte");
    device->setCodenames({ "hlte", "hltecan", "hltespr", "hltetmo", "hlteusc",
                           "hltevzw", "hltexx" });
    device->setName("Samsung Galaxy Note 3 (Snapdragon)");
    device->setBlockDevBaseDirs({ QCOM_BASE_DIR });
    device->setSystemBlockDevs({ QCOM_SYSTEM, "/dev/block/mmcblk0p23" });
    device->setCacheBlockDevs({ QCOM_CACHE, "/dev/block/mmcblk0p24" });
    device->setDataBlockDevs({ QCOM_USERDATA, "/dev/block/mmcblk0p26" });
    device->setBootBlockDevs({ QCOM_BOOT, "/dev/block/mmcblk0p14" });
    device->setRecoveryBlockDevs({ QCOM_RECOVERY });
    devices.push_back(device);

    // Samsung Galaxy Note 3 (Exynos)
    device = new Device();
    device->setId("ha3g");
    device->setCodenames({ "ha3g" });
    device->setName("Samsung Galaxy Note 3 (Exynos)");
    device->setBlockDevBaseDirs({ DWMMC0_BASE_DIR });
    device->setSystemBlockDevs({ DWMMC0_SYSTEM, "/dev/block/mmcblk0p20" });
    device->setCacheBlockDevs({ DWMMC0_CACHE, "/dev/block/mmcblk0p19" });
    device->setDataBlockDevs({ DWMMC0_USERDATA, "/dev/block/mmcblk0p21" });
    device->setBootBlockDevs({ DWMMC0_BOOT, "/dev/block/mmcblk0p9" });
    device->setRecoveryBlockDevs({ DWMMC0_RECOVERY, "/dev/block/mmcblk0p10" });
    device->setExtraBlockDevs({ DWMMC0_RADIO, DWMMC0_CDMA_RADIO });
    devices.push_back(device);

    // Samsung Galaxy Note 3 Neo
    device = new Device();
    device->setId("hllte");
    device->setCodenames({ "hllte", "hlltexx" });
    device->setName("Samsung Galaxy Note 3 Neo");
    device->setBlockDevBaseDirs({ DWMMC0_BASE_DIR });
    device->setSystemBlockDevs({ DWMMC0_SYSTEM, "/dev/block/mmcblk0p18" });
    device->setCacheBlockDevs({ DWMMC0_CACHE, "/dev/block/mmcblk0p19" });
    device->setDataBlockDevs({ DWMMC0_USERDATA, "/dev/block/mmcblk0p21" });
    device->setBootBlockDevs({ DWMMC0_BOOT, "/dev/block/mmcblk0p9" });
    device->setRecoveryBlockDevs({ DWMMC0_RECOVERY, "/dev/block/mmcblk0p10" });
    device->setExtraBlockDevs({ DWMMC0_RADIO, DWMMC0_CDMA_RADIO });
    devices.push_back(device);

    // Samsung Galaxy Note 4 (Snapdragon)
    device = new Device();
    device->setId("trlte");
    device->setCodenames({ "trlte", "trltecan", "trltedt", "trltespr",
                           "trltetmo", "trlteusc", "trltevzw", "trltexx" });
    device->setName("Samsung Galaxy Note 4 (Snapdragon)");
    device->setBlockDevBaseDirs({ QCOM_BASE_DIR });
    device->setSystemBlockDevs({ QCOM_SYSTEM, "/dev/block/mmcblk0p24" });
    device->setCacheBlockDevs({ QCOM_CACHE, "/dev/block/mmcblk0p25" });
    // Shouldn't be an issue as long as ROMs don't touch the "hidden" partition
    device->setDataBlockDevs({ QCOM_USERDATA, "/dev/block/mmcblk0p26",
                               "/dev/block/mmcblk0p27" });
    device->setBootBlockDevs({ QCOM_BOOT, "/dev/block/mmcblk0p17" });
    device->setRecoveryBlockDevs({ QCOM_RECOVERY });
    devices.push_back(device);

    // Samsung Galaxy Note 4 (Exynos)
    device = new Device();
    device->setId("trelte");
    device->setCodenames({
        // N910C
        "trelte", "treltektt", "treltelgt", "trelteskt", "treltexx",
        // N910H
        "tre3g",
        // N910U
        "trhplte"
    });
    device->setName("Samsung Galaxy Note 4 (Exynos)");
    device->setBlockDevBaseDirs({ TRELTE_BASE_DIR });
    device->setSystemBlockDevs({ TRELTE_SYSTEM, "/dev/block/mmcblk0p18" });
    device->setCacheBlockDevs({ TRELTE_CACHE, "/dev/block/mmcblk0p19" });
    device->setDataBlockDevs({ TRELTE_USERDATA, "/dev/block/mmcblk0p21" });
    device->setBootBlockDevs({ TRELTE_BOOT, "/dev/block/mmcblk0p9" });
    device->setRecoveryBlockDevs({ TRELTE_RECOVERY, "/dev/block/mmcblk0p10" });
    device->setExtraBlockDevs({ TRELTE_RADIO, TRELTE_CDMA_RADIO });
    devices.push_back(device);

    // Samsung Galaxy Note 5 (Sprint)
    device = new Device();
    // TODO: May be merged with noblelte once I get the list of partitions from
    //       someone with the device
    device->setId("nobleltespr");
    device->setCodenames({ "noblelte", "nobleltespr" });
    device->setName("Samsung Galaxy Note 5 (Sprint)");
    device->setArchitecture(ARCH_ARM64_V8A);
    // Same named partitions as the S6 variants
    device->setBlockDevBaseDirs({ ZERO_BASE_DIR });
    device->setSystemBlockDevs({ ZERO_SYSTEM, "/dev/block/sda16" });
    device->setCacheBlockDevs({ ZERO_CACHE, "/dev/block/sda17" });
    device->setDataBlockDevs({ ZERO_USERDATA, "/dev/block/sda19" });
    device->setBootBlockDevs({ ZERO_BOOT, "/dev/block/sda7" });
    device->setRecoveryBlockDevs({ ZERO_RECOVERY, "/dev/block/sda8" });
    device->setExtraBlockDevs({ ZERO_RADIO });
    devices.push_back(device);

    // Samsung Galaxy Note 10.1 (2014 Edition)
    device = new Device();
    device->setId("lt03wifi");
    device->setCodenames({ "lt03wifi", "lt03wifiue" });
    device->setName("Samsung Galaxy Note 10.1 (2014 Edition)");
    device->setBlockDevBaseDirs({ DWMMC0_BASE_DIR });
    device->setSystemBlockDevs({ DWMMC0_SYSTEM, "/dev/block/mmcblk0p20" });
    device->setCacheBlockDevs({ DWMMC0_CACHE, "/dev/block/mmcblk0p19" });
    device->setDataBlockDevs({ DWMMC0_USERDATA, "/dev/block/mmcblk0p21" });
    device->setBootBlockDevs({ DWMMC0_BOOT, "/dev/block/mmcblk0p9" });
    device->setRecoveryBlockDevs({ DWMMC0_RECOVERY, "/dev/block/mmcblk0p10" });
    device->setExtraBlockDevs({ DWMMC0_RADIO, DWMMC0_CDMA_RADIO });
    devices.push_back(device);

    // Samsung Galaxy Mega 6.3 (Intl)
    device = new Device();
    device->setId("melius_intl");
    device->setCodenames({ "melius", "meliuslte", "meliusltexx" });
    device->setName("Samsung Galaxy Mega 6.3 (Intl)");
    device->setBlockDevBaseDirs({ QCOM_BASE_DIR });
    device->setSystemBlockDevs({ QCOM_SYSTEM, "/dev/block/mmcblk0p20" });
    device->setCacheBlockDevs({ QCOM_CACHE, "/dev/block/mmcblk0p21" });
    device->setDataBlockDevs({ QCOM_USERDATA, "/dev/block/mmcblk0p23" });
    device->setBootBlockDevs({ QCOM_BOOT, "/dev/block/mmcblk0p13" });
    device->setRecoveryBlockDevs({ QCOM_RECOVERY, "/dev/block/mmcblk0p14" });
    devices.push_back(device);

    // Samsung Galaxy Mega 6.3 (Canada)
    device = new Device();
    device->setId("melius_can");
    device->setCodenames({ "melius", "meliuslte", "meliusltecan" });
    device->setName("Samsung Galaxy Mega 6.3 (Intl)");
    device->setBlockDevBaseDirs({ QCOM_BASE_DIR });
    device->setSystemBlockDevs({ QCOM_SYSTEM, "/dev/block/mmcblk0p21" });
    device->setCacheBlockDevs({ QCOM_CACHE, "/dev/block/mmcblk0p22" });
    device->setDataBlockDevs({ QCOM_USERDATA, "/dev/block/mmcblk0p24" });
    device->setBootBlockDevs({ QCOM_BOOT, "/dev/block/mmcblk0p13" });
    device->setRecoveryBlockDevs({ QCOM_RECOVERY, "/dev/block/mmcblk0p14" });
    devices.push_back(device);

    // Samsung Galaxy Ace 3 LTE
    device = new Device();
    device->setId("loganre");
    device->setCodenames({ "loganre", "loganrelte", "loganreltexx" });
    device->setName("Samsung Galaxy Ace 3 LTE");
    device->setBlockDevBaseDirs({ QCOM_BASE_DIR });
    device->setSystemBlockDevs({ QCOM_SYSTEM, "/dev/block/mmcblk0p20" });
    device->setCacheBlockDevs({ QCOM_CACHE, "/dev/block/mmcblk0p21" });
    device->setDataBlockDevs({ QCOM_USERDATA, "/dev/block/mmcblk0p23" });
    device->setBootBlockDevs({ QCOM_BOOT, "/dev/block/mmcblk0p13" });
    device->setRecoveryBlockDevs({ QCOM_RECOVERY, "/dev/block/mmcblk0p14" });
    devices.push_back(device);

    // Samsung Galaxy Tab 3 10.1
    device = new Device();
    device->setId("santos");
    device->setCodenames({
        // 3G variant
        "santos103g", "santos103gxx",
        // LTE variant
        "santos10lte", "santos10ltexx",
        // Wifi variant
        "santos10wifi", "santos10wifixx"
    });
    device->setName("Samsung Galaxy Tab 3 10.1");
    device->setArchitecture(ARCH_X86);
    device->setBlockDevBaseDirs({ INTEL_PCI_BASE_DIR });
    device->setSystemBlockDevs({ INTEL_PCI_SYSTEM, "/dev/block/mmcblk0p8" });
    device->setCacheBlockDevs({ INTEL_PCI_CACHE, "/dev/block/mmcblk0p6" });
    device->setDataBlockDevs({ INTEL_PCI_USERDATA, "/dev/block/mmcblk0p9" });
    device->setBootBlockDevs({ INTEL_PCI_BOOT, "/dev/block/mmcblk0p10" });
    device->setRecoveryBlockDevs({ INTEL_PCI_RECOVERY,
                                   "/dev/block/mmcblk0p11" });
    device->setExtraBlockDevs({ INTEL_PCI_RADIO });
    devices.push_back(device);

    // Samsung Galaxy Tab 4 10.1 (Wifi)
    device = new Device();
    device->setId("matissewifi");
    device->setCodenames({ "matissewifi", "SM-T530" });
    device->setName("Samsung Galaxy Tab 4 10.1 (Wifi)");
    device->setBlockDevBaseDirs({ QCOM_BASE_DIR });
    device->setSystemBlockDevs({ QCOM_SYSTEM, "/dev/block/mmcblk0p23" });
    device->setCacheBlockDevs({ QCOM_CACHE, "/dev/block/mmcblk0p24" });
    device->setDataBlockDevs({ QCOM_USERDATA, "/dev/block/mmcblk0p26" });
    device->setBootBlockDevs({ QCOM_BOOT, "/dev/block/mmcblk0p14" });
    device->setRecoveryBlockDevs({ QCOM_RECOVERY, "/dev/block/mmcblk0p15" });
    devices.push_back(device);

    // Samsung Galaxy Tab S 8.4 (Wifi)
    device = new Device();
    device->setId("klimtwifi");
    device->setCodenames({ "klimtwifi", "klimtwifikx" });
    device->setName("Samsung Galaxy Tab S 8.4 (Wifi)");
    device->setBlockDevBaseDirs({ DWMMC0_BASE_DIR });
    device->setSystemBlockDevs({ DWMMC0_SYSTEM, "/dev/block/mmcblk0p18" });
    device->setCacheBlockDevs({ DWMMC0_CACHE, "/dev/block/mmcblk0p19" });
    device->setDataBlockDevs({ DWMMC0_USERDATA, "/dev/block/mmcblk0p21" });
    device->setBootBlockDevs({ DWMMC0_BOOT, "/dev/block/mmcblk0p9" });
    device->setRecoveryBlockDevs({ DWMMC0_RECOVERY, "/dev/block/mmcblk0p10" });
    device->setExtraBlockDevs({ DWMMC0_RADIO, DWMMC0_CDMA_RADIO });
    devices.push_back(device);

    // Samsung Galaxy Tab Pro 8.4 (Wifi)
    device = new Device();
    device->setId("mondrianwifi");
    device->setCodenames({ "mondrianwifi", "mondrianwifiue", "mondrianwifixx" });
    device->setName("Samsung Galaxy Tab Pro 8.4 (Wifi)");
    device->setBlockDevBaseDirs({ QCOM_BASE_DIR });
    device->setSystemBlockDevs({ QCOM_SYSTEM, "/dev/block/mmcblk0p23" });
    device->setCacheBlockDevs({ QCOM_CACHE, "/dev/block/mmcblk0p24" });
    device->setDataBlockDevs({ QCOM_USERDATA, "/dev/block/mmcblk0p26" });
    device->setBootBlockDevs({ QCOM_BOOT, "/dev/block/mmcblk0p14" });
    device->setRecoveryBlockDevs({ QCOM_RECOVERY, "/dev/block/mmcblk0p15" });
    devices.push_back(device);

    // Samsung Galaxy Tab Pro 10.1 (Wifi)
    device = new Device();
    device->setId("picassowifi");
    device->setCodenames({ "picassowifi", "picassowifixx" });
    device->setName("Samsung Galaxy Tab Pro 10.1 (Wifi)");
    device->setBlockDevBaseDirs({ DWMMC0_BASE_DIR });
    device->setSystemBlockDevs({ DWMMC0_SYSTEM, "/dev/block/mmcblk0p18" });
    device->setCacheBlockDevs({ DWMMC0_CACHE, "/dev/block/mmcblk0p19" });
    device->setDataBlockDevs({ DWMMC0_USERDATA, "/dev/block/mmcblk0p21" });
    device->setBootBlockDevs({ DWMMC0_BOOT, "/dev/block/mmcblk0p9" });
    device->setRecoveryBlockDevs({ DWMMC0_RECOVERY, "/dev/block/mmcblk0p10" });
    device->setExtraBlockDevs({ DWMMC0_RADIO, DWMMC0_CDMA_RADIO });
    devices.push_back(device);
}

void PatcherConfig::Impl::addLenovoDevices()
{
    Device *device;

    // Lenovo K3 Note
    device = new Device();
    device->setId("k50");
    device->setArchitecture(ARCH_ARM64_V8A);
    device->setCodenames({ "K50", "K50a40", "K50t5", "K50-T5", "aio_otfp" });
    device->setName("Lenovo K3 Note");
    device->setBlockDevBaseDirs({ MTK_BASE_DIR });
    device->setSystemBlockDevs({ MTK_SYSTEM, "/dev/block/mmcblk0p17" });
    device->setCacheBlockDevs({ MTK_CACHE, "/dev/block/mmcblk0p18" });
    device->setDataBlockDevs({ MTK_USERDATA, "/dev/block/mmcblk0p19" });
    device->setBootBlockDevs({ MTK_BOOT, "/dev/block/mmcblk0p7" });
    device->setRecoveryBlockDevs({ MTK_RECOVERY, "/dev/block/mmcblk0p8" });
    devices.push_back(device);
}

void PatcherConfig::Impl::addLgDevices()
{
    Device *device;

    // LG G2
    device = new Device();
    device->setId("lgg2");
    device->setCodenames({ "g2", "d800", "d801", "ls980", "vs980" });
    device->setName("LG G2");
    device->setBlockDevBaseDirs({ QCOM_BASE_DIR });
    device->setSystemBlockDevs({ QCOM_SYSTEM, "/dev/block/mmcblk0p34" });
    device->setCacheBlockDevs({ QCOM_CACHE, "/dev/block/mmcblk0p35" });
    device->setDataBlockDevs({ QCOM_USERDATA, "/dev/block/mmcblk0p38" });
    device->setBootBlockDevs({ QCOM_BOOT, "/dev/block/mmcblk0p7" });
    device->setRecoveryBlockDevs({ QCOM_RECOVERY, "/dev/block/mmcblk0p15" });
    device->setExtraBlockDevs({ QCOM_ABOOT, QCOM_TZ });
    devices.push_back(device);

    // LG G2 (d802)
    device = new Device();
    device->setId("lgg2_d802");
    device->setCodenames({ "g2", "d802", "d802t" });
    device->setName("LG G2 (d802)");
    device->setBlockDevBaseDirs({ QCOM_BASE_DIR });
    device->setSystemBlockDevs({ QCOM_SYSTEM, "/dev/block/mmcblk0p30" });
    device->setCacheBlockDevs({ QCOM_CACHE, "/dev/block/mmcblk0p31" });
    device->setDataBlockDevs({ QCOM_USERDATA, "/dev/block/mmcblk0p35" });
    device->setBootBlockDevs({ QCOM_BOOT, "/dev/block/mmcblk0p7" });
    device->setRecoveryBlockDevs({ QCOM_RECOVERY, "/dev/block/mmcblk0p15" });
    device->setExtraBlockDevs({ QCOM_ABOOT, QCOM_TZ });
    devices.push_back(device);

    // LG G3
    device = new Device();
    device->setId("lgg3");
    device->setCodenames({ "g3", "d850", "d851", "d852", "d855", "f400",
                           "f400k", "ls990", "vs985" });
    device->setName("LG G3");
    device->setBlockDevBaseDirs({ QCOM_BASE_DIR });
    device->setSystemBlockDevs({ QCOM_SYSTEM, "/dev/block/mmcblk0p40" });
    device->setCacheBlockDevs({ QCOM_CACHE, "/dev/block/mmcblk0p41" });
    device->setDataBlockDevs({ QCOM_USERDATA, "/dev/block/mmcblk0p43" });
    device->setBootBlockDevs({ QCOM_BOOT, "/dev/block/mmcblk0p18" });
    device->setRecoveryBlockDevs({ QCOM_RECOVERY });
    device->setExtraBlockDevs({ QCOM_ABOOT, QCOM_MODEM });
    devices.push_back(device);

    // LG G4
    device = new Device();
    device->setId("lgg4");
    device->setArchitecture(ARCH_ARM64_V8A);
    device->setCodenames({ "p1", "h815" });
    device->setName("LG G4");
    device->setBlockDevBaseDirs({ F9824900_BASE_DIR, BOOTDEVICE_BASE_DIR });
    device->setSystemBlockDevs({ F9824900_SYSTEM, BOOTDEVICE_SYSTEM,
                                 "/dev/block/mmcblk0p47" });
    device->setCacheBlockDevs({ F9824900_CACHE, BOOTDEVICE_CACHE,
                                "/dev/block/mmcblk0p49" });
    device->setDataBlockDevs({ F9824900_USERDATA, BOOTDEVICE_USERDATA,
                               "/dev/block/mmcblk0p50" });
    device->setBootBlockDevs({ F9824900_BOOT, BOOTDEVICE_BOOT,
                               "/dev/block/mmcblk0p38" });
    device->setRecoveryBlockDevs({ F9824900_RECOVERY, BOOTDEVICE_RECOVERY,
                                   "/dev/block/mmcblk0p39" });
    devices.push_back(device);
}

void PatcherConfig::Impl::addMotorolaDevices()
{
    Device *device;

    // Motorola Moto G (2013)
    device = new Device();
    device->setId("falcon");
    device->setCodenames({ "falcon", "falcon_umts", "falcon_umtsds",
                           "xt1032" });
    device->setName("Motorola Moto G (2013)");
    device->setBlockDevBaseDirs({ QCOM_BASE_DIR });
    device->setSystemBlockDevs({ QCOM_SYSTEM, "/dev/block/mmcblk0p34" });
    device->setCacheBlockDevs({ QCOM_CACHE, "/dev/block/mmcblk0p33" });
    device->setDataBlockDevs({ QCOM_USERDATA, "/dev/block/mmcblk0p36" });
    device->setBootBlockDevs({ QCOM_BOOT, "/dev/block/mmcblk0p31" });
    device->setRecoveryBlockDevs({ QCOM_RECOVERY, "/dev/block/mmcblk0p32" });
    devices.push_back(device);

    // Motorola Moto X (2013)
    device = new Device();
    device->setId("ghost");
    device->setCodenames({ "ghost", "ghost_att", "ghost_rcica", "ghost_retail",
                           "ghost_sprint", "ghost_usc", "ghost_verizon",
                           "xt1052", "xt1053", "xt1055", "xt1056", "xt1058",
                           "xt1060", });
    device->setName("Motorola Moto X (2013)");
    device->setBlockDevBaseDirs({ QCOM_BASE_DIR });
    device->setSystemBlockDevs({ QCOM_SYSTEM, "/dev/block/mmcblk0p38" });
    device->setCacheBlockDevs({ QCOM_CACHE, "/dev/block/mmcblk0p36" });
    device->setDataBlockDevs({ QCOM_USERDATA, "/dev/block/mmcblk0p40" });
    device->setBootBlockDevs({ QCOM_BOOT, "/dev/block/mmcblk0p33" });
    device->setRecoveryBlockDevs({ QCOM_RECOVERY });
    devices.push_back(device);

    // Motorola Nexus 6
    device = new Device();
    device->setId("shamu");
    device->setCodenames({ "shamu" });
    device->setName("Motorola Nexus 6");
    device->setBlockDevBaseDirs({ QCOM_BASE_DIR });
    device->setSystemBlockDevs({ QCOM_SYSTEM, "/dev/block/mmcblk0p41" });
    device->setCacheBlockDevs({ QCOM_CACHE, "/dev/block/mmcblk0p38" });
    device->setDataBlockDevs({ QCOM_USERDATA, "/dev/block/mmcblk0p42" });
    device->setBootBlockDevs({ QCOM_BOOT, "/dev/block/mmcblk0p37" });
    device->setRecoveryBlockDevs({ QCOM_RECOVERY, "/dev/block/mmcblk0p35" });
    devices.push_back(device);
}

void PatcherConfig::Impl::addNexusDevices()
{
    Device *device;

    // Google/LG Nexus 4
    device = new Device();
    device->setId("mako");
    device->setCodenames({ "mako" });
    device->setName("Google/LG Nexus 4");
    device->setBlockDevBaseDirs({ QCOM_BASE_DIR });
    device->setSystemBlockDevs({ QCOM_SYSTEM, "/dev/block/mmcblk0p21" });
    device->setCacheBlockDevs({ QCOM_CACHE, "/dev/block/mmcblk0p22" });
    device->setDataBlockDevs({ QCOM_USERDATA, "/dev/block/mmcblk0p23" });
    device->setBootBlockDevs({ QCOM_BOOT, "/dev/block/mmcblk0p6" });
    device->setRecoveryBlockDevs({ QCOM_RECOVERY, "/dev/block/mmcblk0p7" });
    devices.push_back(device);

    // Google/LG Nexus 5
    device = new Device();
    device->setId("hammerhead");
    device->setCodenames({ "hammerhead" });
    device->setName("Google/LG Nexus 5");
    device->setBlockDevBaseDirs({ QCOM_BASE_DIR });
    device->setSystemBlockDevs({ QCOM_SYSTEM, "/dev/block/mmcblk0p25" });
    device->setCacheBlockDevs({ QCOM_CACHE, "/dev/block/mmcblk0p27" });
    device->setDataBlockDevs({ QCOM_USERDATA, "/dev/block/mmcblk0p28" });
    device->setBootBlockDevs({ QCOM_BOOT, "/dev/block/mmcblk0p19" });
    device->setExtraBlockDevs({ QCOM_ABOOT, QCOM_IMGDATA, QCOM_MISC, QCOM_MODEM,
                                QCOM_RPM, QCOM_SBL1, QCOM_SDI, QCOM_TZ });
    device->setRecoveryBlockDevs({ QCOM_RECOVERY });
    devices.push_back(device);

    // Google/ASUS Nexus 7 (2012 Wifi)
    device = new Device();
    device->setId("grouper");
    device->setCodenames({ "grouper" });
    device->setName("Google/ASUS Nexus 7 (2012 Wifi)");
    device->setBlockDevBaseDirs({ TEGRA3_BASE_DIR });
    device->setSystemBlockDevs({ TEGRA3_SYSTEM, "/dev/block/mmcblk0p3" });
    device->setCacheBlockDevs({ TEGRA3_CACHE, "/dev/block/mmcblk0p4" });
    device->setDataBlockDevs({ TEGRA3_USERDATA, "/dev/block/mmcblk0p9" });
    device->setBootBlockDevs({ TEGRA3_BOOT, "/dev/block/mmcblk0p2" });
    device->setRecoveryBlockDevs({ TEGRA3_RECOVERY, "/dev/block/mmcblk0p1" });
    devices.push_back(device);

    // Google/ASUS Nexus 7 (2013 Wifi)
    device = new Device();
    device->setId("flo");
    device->setCodenames({ "flo" });
    device->setName("Google/ASUS Nexus 7 (2013 Wifi)");
    device->setBlockDevBaseDirs({ QCOM_BASE_DIR });
    device->setSystemBlockDevs({ QCOM_SYSTEM, "/dev/block/mmcblk0p22" });
    device->setCacheBlockDevs({ QCOM_CACHE, "/dev/block/mmcblk0p23" });
    device->setDataBlockDevs({ QCOM_USERDATA, "/dev/block/mmcblk0p30" });
    device->setBootBlockDevs({ QCOM_BOOT, "/dev/block/mmcblk0p14" });
    device->setRecoveryBlockDevs({ QCOM_RECOVERY });
    devices.push_back(device);
}

void PatcherConfig::Impl::addOnePlusDevices()
{
    Device *device;

    // OnePlus One
    device = new Device();
    device->setId("bacon");
    device->setCodenames({ "bacon", "A0001" });
    device->setName("OnePlus One");
    device->setBlockDevBaseDirs({ QCOM_BASE_DIR });
    device->setSystemBlockDevs({ QCOM_SYSTEM, "/dev/block/mmcblk0p14" });
    device->setCacheBlockDevs({ QCOM_CACHE, "/dev/block/mmcblk0p16" });
    device->setDataBlockDevs({ QCOM_USERDATA, "/dev/block/mmcblk0p28" });
    device->setBootBlockDevs({ QCOM_BOOT, "/dev/block/mmcblk0p7" });
    device->setRecoveryBlockDevs({ QCOM_RECOVERY });
    device->setExtraBlockDevs({ QCOM_TZ, "/dev/block/mmcblk0p8" });
    devices.push_back(device);

    // OnePlus Two
    device = new Device();
    device->setId("OnePlus2");
    device->setArchitecture(ARCH_ARM64_V8A);
    device->setCodenames({ "OnePlus2" });
    device->setName("OnePlus Two");
    device->setBlockDevBaseDirs({ F9824900_BASE_DIR, BOOTDEVICE_BASE_DIR });
    device->setSystemBlockDevs({ F9824900_SYSTEM, BOOTDEVICE_SYSTEM,
                                 "/dev/block/mmcblk0p42" });
    device->setCacheBlockDevs({ F9824900_CACHE, BOOTDEVICE_CACHE,
                                "/dev/block/mmcblk0p41" });
    device->setDataBlockDevs({ F9824900_USERDATA, BOOTDEVICE_USERDATA,
                               "/dev/block/mmcblk0p43" });
    device->setBootBlockDevs({ F9824900_BOOT, BOOTDEVICE_BOOT,
                               "/dev/block/mmcblk0p35" });
    device->setRecoveryBlockDevs({ F9824900_RECOVERY, BOOTDEVICE_RECOVERY,
                                   "/dev/block/mmcblk0p36" });
    devices.push_back(device);
}

void PatcherConfig::Impl::addHuaweiDevices()
{
    Device *device;

    // Huawei Mate 2
    device = new Device();
    device->setId("mt2l03");
    device->setCodenames({ "hwMT2L03", "hwMT2LO3", "mt2", "MT2", "mt2l03",
                           "MT2L03", "mt2-l03", "MT2-L03" });
    device->setName("Huawei Ascend Mate 2");
    device->setBlockDevBaseDirs({ QCOM_BASE_DIR });
    device->setSystemBlockDevs({ QCOM_SYSTEM, "/dev/block/mmcblk0p23" });
    device->setCacheBlockDevs({ QCOM_CACHE, "/dev/block/mmcblk0p21" });
    device->setDataBlockDevs({ QCOM_USERDATA, "/dev/block/mmcblk0p24" });
    device->setBootBlockDevs({ QCOM_BOOT, "/dev/block/mmcblk0p18" });
    device->setRecoveryBlockDevs({ QCOM_RECOVERY, "/dev/block/mmcblk0p19" });
    devices.push_back(device);

    // Huawei Ascend P7
    device = new Device();
    device->setId("hwp7");
    device->setCodenames({ "hwp7" });
    device->setName("Huawei Ascend P7");
    device->setBlockDevBaseDirs({ HISILICON_BASE_DIR });
    device->setSystemBlockDevs({ HISILICON_SYSTEM, "/dev/block/mmcblk0p28" });
    device->setCacheBlockDevs({ HISILICON_CACHE, "/dev/block/mmcblk0p21" });
    device->setDataBlockDevs({ HISILICON_USERDATA, "/dev/block/mmcblk0p30" });
    device->setBootBlockDevs({ HISILICON_BOOT, "/dev/block/mmcblk0p17" });
    device->setRecoveryBlockDevs({ HISILICON_RECOVERY, "/dev/block/mmcblk0p18" });
    devices.push_back(device);
}

void PatcherConfig::Impl::addSonyDevices()
{
    Device *device;

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

void PatcherConfig::Impl::loadDefaultDevices()
{
    addSamsungDevices();
    addLenovoDevices();
    addLgDevices();
    addMotorolaDevices();
    addNexusDevices();
    addOnePlusDevices();
    addHuaweiDevices();
    addSonyDevices();
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
        StandardPatcher::Id,
        XposedPatcher::Id
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
        DefaultRP::Id,
        PepperDefaultRP::Id,
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
 *
 * \return New AutoPatcher
 */
AutoPatcher * PatcherConfig::createAutoPatcher(const std::string &id,
                                               const FileInfo * const info)
{
    AutoPatcher *ap = nullptr;

    if (id == StandardPatcher::Id) {
        ap = new StandardPatcher(this, info);
    } else if (id == XposedPatcher::Id) {
        ap = new XposedPatcher(this, info);
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

    if (id == DefaultRP::Id) {
        rp = new DefaultRP(this, info, cpio);
    } else if (id == PepperDefaultRP::Id) {
        rp = new PepperDefaultRP(this, info, cpio);
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

#endif

}
