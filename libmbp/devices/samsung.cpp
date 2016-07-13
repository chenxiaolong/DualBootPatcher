/*
 * Copyright (C) 2015-2016  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "devices/samsung.h"

#include "devices/paths.h"

/*
 * NOTE: Samsung has 10 billion devices, but try to keep the functions below as
 * small as possible. Split devices to separate functions if needed. Otherwise,
 * the compiler's variable tracking will use a whole lot of memory and will be
 * very slow.
 */

namespace mbp
{

static void addGalaxyNoteSeriesPhones(std::vector<Device *> *devices)
{
    Device *device;

    // Samsung Galaxy Note 2
    device = new Device();
    device->setId("t0lte");
    device->setCodenames({ "t0lte", "t0lteatt", "t0ltecan", "t0ltelgt",
        /* "t0ltespr", */ "t0ltetmo", /* t0lteusc, */ "t0ltevzw" });
    device->setName("Samsung Galaxy Note 2");
    device->setBlockDevBaseDirs({ DWMMC_BASE_DIR });
    device->setSystemBlockDevs({ DWMMC_SYSTEM, "/dev/block/mmcblk0p13" });
    device->setCacheBlockDevs({ DWMMC_CACHE, "/dev/block/mmcblk0p12" });
    device->setDataBlockDevs({ DWMMC_USERDATA, "/dev/block/mmcblk0p16" });
    device->setBootBlockDevs({ DWMMC_BOOT, "/dev/block/mmcblk0p8" });
    device->setRecoveryBlockDevs({ DWMMC_RECOVERY, "/dev/block/mmcblk0p9" });
    device->setExtraBlockDevs({ DWMMC_RADIO });
    devices->push_back(device);

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
    device->twOptions()->supported = true;
    device->twOptions()->graphicsBackends = { "fbdev" };
    device->twOptions()->flags = Device::FLAG_TW_QCOM_RTC_FIX | Device::FLAG_TW_HAS_DOWNLOAD_MODE;
    device->twOptions()->pixelFormat = Device::TwPixelFormat::RGBX_8888;
    device->twOptions()->brightnessPath = "/sys/devices/mdp.0/qcom,mdss_fb_primary.185/leds/lcd-backlight/brightness";
    device->twOptions()->maxBrightness = 255;
    device->twOptions()->defaultBrightness = 162;
    devices->push_back(device);

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
    devices->push_back(device);

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
    devices->push_back(device);

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
    devices->push_back(device);

    // Samsung Galaxy Note 4 (Exynos)
    device = new Device();
    device->setId("trelte");
    device->setCodenames({
        // N910C
        "trelte", "treltektt", "treltelgt", "trelteskt", "treltexx",
        // N910H
        "tre3g",
        // N910U
        "trhplte" });
    device->setName("Samsung Galaxy Note 4 (Exynos)");
    device->setBlockDevBaseDirs({ DWMMC0_15540000_BASE_DIR });
    device->setSystemBlockDevs({ DWMMC0_15540000_SYSTEM,
        "/dev/block/mmcblk0p18" });
    device->setCacheBlockDevs({ DWMMC0_15540000_CACHE,
        "/dev/block/mmcblk0p19" });
    device->setDataBlockDevs({ DWMMC0_15540000_USERDATA,
        "/dev/block/mmcblk0p21" });
    device->setBootBlockDevs({ DWMMC0_15540000_BOOT,
        "/dev/block/mmcblk0p9" });
    device->setRecoveryBlockDevs({ DWMMC0_15540000_RECOVERY,
        "/dev/block/mmcblk0p10" });
    device->setExtraBlockDevs({ DWMMC0_15540000_RADIO,
        DWMMC0_15540000_CDMA_RADIO });
    devices->push_back(device);

    // Samsung Galaxy Note 5 (Sprint)
    device = new Device();
    // TODO: May be merged with noblelte once I get the list of partitions from
    //       someone with the device
    device->setId("nobleltespr");
    device->setCodenames({ "noblelte", "nobleltespr" });
    device->setName("Samsung Galaxy Note 5 (Sprint)");
    device->setArchitecture(ARCH_ARM64_V8A);
    device->setBlockDevBaseDirs({ UFS_15570000_BASE_DIR });
    device->setSystemBlockDevs({ UFS_15570000_SYSTEM, "/dev/block/sda16" });
    device->setCacheBlockDevs({ UFS_15570000_CACHE, "/dev/block/sda17" });
    device->setDataBlockDevs({ UFS_15570000_USERDATA, "/dev/block/sda19" });
    device->setBootBlockDevs({ UFS_15570000_BOOT, "/dev/block/sda7" });
    device->setRecoveryBlockDevs({ UFS_15570000_RECOVERY, "/dev/block/sda8" });
    device->setExtraBlockDevs({ UFS_15570000_RADIO });
    devices->push_back(device);
}

static void addGalaxySSeriesPhones(std::vector<Device *> *devices)
{
    Device *device;

    // Samsung Galaxy S 3 Neo (qcom)
    device = new Device();
    device->setId("s3ve3g");
    device->setCodenames({ "s3ve3g", "s3ve3gds", "s3ve3gdd", "s3ve3jv" });
    device->setName("Samsung Galaxy S 3 Neo (Qcom)");
    device->setBlockDevBaseDirs({ QCOM_BASE_DIR });
    device->setSystemBlockDevs({ QCOM_SYSTEM, "/dev/block/mmcblk0p23" });
    device->setCacheBlockDevs({ QCOM_CACHE, "/dev/block/mmcblk0p24" });
    device->setDataBlockDevs({ QCOM_USERDATA, "/dev/block/mmcblk0p26" });
    device->setBootBlockDevs({ QCOM_BOOT, "/dev/block/mmcblk0p14" });
    device->setRecoveryBlockDevs({ QCOM_RECOVERY, "/dev/block/mmcblk0p15" });
    devices->push_back(device);

    // Samsung Galaxy S 3 (Qcom)
    device = new Device();
    device->setId("d2");
    device->setCodenames({ "d2", "d2lte", "d2att", "d2can", "d2cri", "d2ltetmo",
        "d2mtr", "d2spi", "d2spr", "d2tfnspr", "d2tfnvzw", "d2tmo", "d2usc",
        "d2vmu", "d2vzw", "d2xar" });
    device->setName("Samsung Galaxy S 3 (Qcom)");
    device->setBlockDevBaseDirs({ QCOM_BASE_DIR });
    device->setSystemBlockDevs({ QCOM_SYSTEM, "/dev/block/mmcblk0p14" });
    device->setCacheBlockDevs({ QCOM_CACHE, "/dev/block/mmcblk0p17" });
    device->setDataBlockDevs({ QCOM_USERDATA, "/dev/block/mmcblk0p15" });
    device->setBootBlockDevs({ QCOM_BOOT, "/dev/block/mmcblk0p7" });
    device->setRecoveryBlockDevs({ QCOM_RECOVERY, "/dev/block/mmcblk0p18" });
    device->setExtraBlockDevs({ QCOM_ABOOT });
    devices->push_back(device);

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
    devices->push_back(device);

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
    devices->push_back(device);

    // Samsung Galaxy S 3 Mini
    device = new Device();
    device->setId("golden");
    device->setCodenames({ "golden" });
    device->setName("Samsung Galaxy S 3 Mini");
    device->setSystemBlockDevs({ "/dev/block/mmcblk0p22" });
    device->setCacheBlockDevs({ "/dev/block/mmcblk0p23" });
    device->setDataBlockDevs({ "/dev/block/mmcblk0p25" });
    device->setBootBlockDevs({ "/dev/block/mmcblk0p20" });
    device->setRecoveryBlockDevs({ "/dev/block/mmcblk0p21" });
    devices->push_back(device);

    // Samsung Galaxy S 4 (Qcom)
    device = new Device();
    device->setId("jflte");
    device->setCodenames({
        // Regular variant
        "jflte", "jflteatt", "jfltecan", "jfltecri", "jfltecsp",
        "jflterefreshspr", "jfltespr", "jfltetmo", "jflteusc", "jfltevzw",
        "jfltexx", "jfltezm",
        // Active variant
        "jactivelte",
        // Google Edition variant
        "jgedlte",
        // GT-I9507
        "jftddxx",
        // GT-I9515
        "jfvelte", "jfveltexx" });
    device->setName("Samsung Galaxy S 4 (Qcom)");
    device->setBlockDevBaseDirs({ QCOM_BASE_DIR });
    device->setSystemBlockDevs({ QCOM_SYSTEM, "/dev/block/mmcblk0p16" });
    device->setCacheBlockDevs({ QCOM_CACHE, "/dev/block/mmcblk0p18" });
    device->setDataBlockDevs({ QCOM_USERDATA, "/dev/block/mmcblk0p29" });
    device->setBootBlockDevs({ QCOM_BOOT, "/dev/block/mmcblk0p20" });
    device->setRecoveryBlockDevs({ QCOM_RECOVERY, "/dev/block/mmcblk0p21" });
    device->setExtraBlockDevs({ QCOM_ABOOT });
    device->twOptions()->supported = true;
    device->twOptions()->flags = Device::FLAG_TW_PREFER_LCD_BACKLIGHT
        | Device::FLAG_TW_HAS_DOWNLOAD_MODE;
    device->twOptions()->graphicsBackends = { "fbdev" };
    devices->push_back(device);

    // Samsung Galaxy S 4 (Exynos)
    device = new Device();
    device->setId("i9500");
    device->setCodenames({
        // i9500
        "ja3g", "jalte",
        // SHV-E300S/K
        "jaltektt", "jalteskt",
        // For OpenRecovery (TWRP)
        "i9500" });
    device->setName("Samsung Galaxy S 4 (Exynos)");
    device->setBlockDevBaseDirs({ DWMMC0_BASE_DIR });
    device->setSystemBlockDevs({ DWMMC0_SYSTEM, "/dev/block/mmcblk0p20" });
    device->setCacheBlockDevs({ DWMMC0_CACHE, "/dev/block/mmcblk0p19" });
    device->setDataBlockDevs({ DWMMC0_USERDATA, "/dev/block/mmcblk0p21" });
    device->setBootBlockDevs({ DWMMC0_BOOT, "/dev/block/mmcblk0p9" });
    device->setRecoveryBlockDevs({ DWMMC0_RECOVERY, "/dev/block/mmcblk0p10" });
    device->setExtraBlockDevs({ DWMMC0_RADIO, DWMMC0_CDMA_RADIO });
    devices->push_back(device);

    // Samsung Galaxy S 4 LTE-A
    device = new Device();
    device->setId("ks01lte");
    device->setCodenames({ "ks01lte", "ks01ltektt", "ks01lteskt" });
    device->setName("Samsung Galaxy S 4 LTE-A");
    device->setBlockDevBaseDirs({ QCOM_BASE_DIR });
    device->setSystemBlockDevs({ QCOM_SYSTEM, "/dev/block/mmcblk0p23" });
    device->setCacheBlockDevs({ QCOM_CACHE, "/dev/block/mmcblk0p24" });
    device->setDataBlockDevs({ QCOM_USERDATA, "/dev/block/mmcblk0p26" });
    device->setBootBlockDevs({ QCOM_BOOT, "/dev/block/mmcblk0p14" });
    device->setRecoveryBlockDevs({ QCOM_RECOVERY, "/dev/block/mmcblk0p15" });
    devices->push_back(device);

    // Samsung Galaxy S 4 Active (ONLY for SKT variant)
    device = new Device();
    device->setId("jactivelteskt");
    device->setCodenames({ "jactivelteskt" });
    device->setName("Samsung Galaxy S 4 Active (SKT Variant Only)");
    device->setBlockDevBaseDirs({ QCOM_BASE_DIR });
    device->setSystemBlockDevs({ QCOM_SYSTEM, "/dev/block/mmcblk0p23" });
    device->setCacheBlockDevs({ QCOM_CACHE, "/dev/block/mmcblk0p24" });
    device->setDataBlockDevs({ QCOM_USERDATA, "/dev/block/mmcblk0p25" });
    device->setBootBlockDevs({ QCOM_BOOT, "/dev/block/mmcblk0p14" });
    device->setRecoveryBlockDevs({ QCOM_RECOVERY, "/dev/block/mmcblk0p15" });
    devices->push_back(device);

    // Samsung Galaxy S 4 Mini
    device = new Device();
    device->setId("serrano");
    device->setCodenames({
        // Regular variant
        "serrano3g", "serrano3gxx",
        // Duos variant
        "serranods", "serranodsxx",
        // LTE variant
        "serranolte", "serranoltexx" });
    device->setName("Samsung Galaxy S 4 Mini");
    device->setBlockDevBaseDirs({ QCOM_BASE_DIR });
    device->setSystemBlockDevs({ QCOM_SYSTEM, "/dev/block/mmcblk0p21" });
    device->setCacheBlockDevs({ QCOM_CACHE, "/dev/block/mmcblk0p22" });
    device->setDataBlockDevs({ QCOM_USERDATA, "/dev/block/mmcblk0p24" });
    device->setBootBlockDevs({ QCOM_BOOT, "/dev/block/mmcblk0p13" });
    device->setRecoveryBlockDevs({ QCOM_RECOVERY });
    devices->push_back(device);

    // Samsung Galaxy S 5 (Qcom)
    device = new Device();
    device->setId("klte");
    device->setCodenames({ "klte", "kltecan", "kltedv", "kltespr", "kltetmo",
        "klteusc", "kltevzw", "kltexx", "kltekdi" });
    device->setName("Samsung Galaxy S 5 (Qcom)");
    device->setBlockDevBaseDirs({ QCOM_BASE_DIR });
    device->setSystemBlockDevs({ QCOM_SYSTEM, "/dev/block/mmcblk0p23" });
    device->setCacheBlockDevs({ QCOM_CACHE, "/dev/block/mmcblk0p24" });
    device->setDataBlockDevs({ QCOM_USERDATA, "/dev/block/mmcblk0p26" });
    device->setBootBlockDevs({ QCOM_BOOT, "/dev/block/mmcblk0p15" });
    device->setRecoveryBlockDevs({ QCOM_RECOVERY, "/dev/block/mmcblk0p14" });
    devices->push_back(device);

    // Samsung Galaxy S 5 mini (Qcom)
    device = new Device();
    device->setId("kmini3g");
    device->setCodenames({ "kmini3g" });
    device->setName("Samsung Galaxy S 5 Mini (Qcom)");
    device->setBlockDevBaseDirs({ QCOM_BASE_DIR });
    device->setSystemBlockDevs({ QCOM_SYSTEM, "/dev/block/mmcblk0p23" });
    device->setCacheBlockDevs({ QCOM_CACHE, "/dev/block/mmcblk0p24" });
    device->setDataBlockDevs({ QCOM_USERDATA, "/dev/block/mmcblk0p26" });
    device->setBootBlockDevs({ QCOM_BOOT, "/dev/block/mmcblk0p14" });
    device->setRecoveryBlockDevs({ QCOM_RECOVERY, "/dev/block/mmcblk0p15" });
    devices->push_back(device);

    // Samsung Galaxy S 5 (Exynos)
    device = new Device();
    device->setId("k3g");
    device->setCodenames({ "k3g", "k3gxx" });
    device->setName("Samsung Galaxy S 5 (Exynos)");
    device->setBlockDevBaseDirs({ DWMMC0_12200000_BASE_DIR });
    device->setSystemBlockDevs({ DWMMC0_12200000_SYSTEM,
        "/dev/block/mmcblk0p18" });
    device->setCacheBlockDevs({ DWMMC0_12200000_CACHE,
        "/dev/block/mmcblk0p19" });
    device->setDataBlockDevs({ DWMMC0_12200000_USERDATA,
        "/dev/block/mmcblk0p21" });
    device->setBootBlockDevs({ DWMMC0_12200000_BOOT,
        "/dev/block/mmcblk0p9" });
    device->setRecoveryBlockDevs({ DWMMC0_12200000_RECOVERY,
        "/dev/block/mmcblk0p10" });
    device->setExtraBlockDevs({ DWMMC0_12200000_RADIO,
        DWMMC0_12200000_CDMA_RADIO });
    devices->push_back(device);

    // Samsung Galaxy S 5 mini lte (Exynos)
    device = new Device();
    device->setId("kminilte");
    device->setCodenames({ "kminilte", "kminiltexx" });
    device->setName("Samsung Galaxy S 5 Mini lte (Exynos)");
    device->setBlockDevBaseDirs({ DWMMC0_BASE_DIR });
    device->setSystemBlockDevs({ DWMMC0_SYSTEM, "/dev/block/mmcblk0p18" });
    device->setCacheBlockDevs({ DWMMC0_CACHE, "/dev/block/mmcblk0p19" });
    device->setDataBlockDevs({ DWMMC0_USERDATA, "/dev/block/mmcblk0p21" });
    device->setBootBlockDevs({ DWMMC0_BOOT, "/dev/block/mmcblk0p9" });
    device->setRecoveryBlockDevs({ DWMMC0_RECOVERY, "/dev/block/mmcblk0p10" });
    device->setExtraBlockDevs({ DWMMC0_RADIO, DWMMC0_CDMA_RADIO });
    devices->push_back(device);

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
    devices->push_back(device);

    // Samsung Galaxy S 5 4G+
    device = new Device();
    device->setId("kccat6");
    device->setCodenames({ "kccat6", "kccat6xx" });
    device->setName("Samsung Galaxy S 5 4G+");
    device->setBlockDevBaseDirs({ QCOM_BASE_DIR });
    device->setSystemBlockDevs({ QCOM_SYSTEM, "/dev/block/mmcblk0p24" });
    device->setCacheBlockDevs({ QCOM_CACHE, "/dev/block/mmcblk0p25" });
    device->setDataBlockDevs({ QCOM_USERDATA, "/dev/block/mmcblk0p27" });
    device->setBootBlockDevs({ QCOM_BOOT, "/dev/block/mmcblk0p17" });
    device->setRecoveryBlockDevs({ QCOM_RECOVERY, "/dev/block/mmcblk0p18" });
    devices->push_back(device);

    // Samsung Galaxy S 6 Flat/Edge
    device = new Device();
    device->setId("zerolte");
    device->setCodenames({
        // Regular variant
        "zeroflte", "zerofltebmc", "zerofltetmo", "zerofltexx",
        // Edge variant
        "zerolte", "zeroltebmc", "zeroltetmo", "zeroltexx" });
    device->setName("Samsung Galaxy S 6 Flat/Edge");
    device->setArchitecture(ARCH_ARM64_V8A);
    device->setBlockDevBaseDirs({ UFS_15570000_BASE_DIR });
    device->setSystemBlockDevs({ UFS_15570000_SYSTEM, "/dev/block/sda15" });
    device->setCacheBlockDevs({ UFS_15570000_CACHE, "/dev/block/sda16" });
    device->setDataBlockDevs({ UFS_15570000_USERDATA, "/dev/block/sda17" });
    device->setBootBlockDevs({ UFS_15570000_BOOT, "/dev/block/sda5" });
    device->setRecoveryBlockDevs({ UFS_15570000_RECOVERY, "/dev/block/sda6" });
    device->setExtraBlockDevs({ UFS_15570000_RADIO });
    devices->push_back(device);

    // Samsung Galaxy S 6 Flat/Edge (Sprint)
    device = new Device();
    device->setId("zeroltespr");
    device->setCodenames({
        // Regular variant
        "zerofltespr",
        // Edge variant
        "zeroltespr" });
    device->setName("Samsung Galaxy S 6 Flat/Edge (Sprint)");
    device->setArchitecture(ARCH_ARM64_V8A);
    device->setBlockDevBaseDirs({ UFS_15570000_BASE_DIR });
    device->setSystemBlockDevs({ UFS_15570000_SYSTEM, "/dev/block/sda18" });
    device->setCacheBlockDevs({ UFS_15570000_CACHE, "/dev/block/sda19" });
    device->setDataBlockDevs({ UFS_15570000_USERDATA, "/dev/block/sda21" });
    device->setBootBlockDevs({ UFS_15570000_BOOT, "/dev/block/sda8" });
    device->setRecoveryBlockDevs({ UFS_15570000_RECOVERY, "/dev/block/sda9" });
    device->setExtraBlockDevs({ UFS_15570000_RADIO });
    devices->push_back(device);

    // Samsung Galaxy S 6 Edge+
    device = new Device();
    device->setId("zenlte");
    device->setCodenames({ "zenlte", "zenltexx" });
    device->setName("Samsung Galaxy S 6 Edge+");
    device->setArchitecture(ARCH_ARM64_V8A);
    device->setBlockDevBaseDirs({ UFS_15570000_BASE_DIR });
    device->setSystemBlockDevs({ UFS_15570000_SYSTEM, "/dev/block/sda14" });
    device->setCacheBlockDevs({ UFS_15570000_CACHE, "/dev/block/sda15" });
    device->setDataBlockDevs({ UFS_15570000_USERDATA, "/dev/block/sda17" });
    device->setBootBlockDevs({ UFS_15570000_BOOT, "/dev/block/sda5" });
    device->setRecoveryBlockDevs({ UFS_15570000_RECOVERY, "/dev/block/sda6" });
    device->setExtraBlockDevs({ UFS_15570000_RADIO });
    devices->push_back(device);

    // Samsung Galaxy S 7 Flat/Edge (Exynos)
    device = new Device();
    device->setId("herolte");
    device->setCodenames({
        // Regular variant
        "herolte", "heroltexx", "heroltebmc", "heroltektt", "heroltelgt",
        "herolteskt",
        // Edge variant
        "hero2lte", "hero2ltexx", "hero2ltebmc", "hero2ltektt", "hero2ltelgt",
        "hero2lteskt" });
    device->setName("Samsung Galaxy S 7 Flat/Edge (Exynos)");
    device->setArchitecture(ARCH_ARM64_V8A);
    device->setBlockDevBaseDirs({ UFS_155A0000_BASE_DIR });
    device->setSystemBlockDevs({ UFS_155A0000_SYSTEM, "/dev/block/sda14" });
    device->setCacheBlockDevs({ UFS_155A0000_CACHE, "/dev/block/sda15" });
    device->setDataBlockDevs({ UFS_155A0000_USERDATA, "/dev/block/sda18" });
    device->setBootBlockDevs({ UFS_155A0000_BOOT, "/dev/block/sda5" });
    device->setRecoveryBlockDevs({ UFS_155A0000_RECOVERY, "/dev/block/sda6" });
    device->setExtraBlockDevs({ UFS_155A0000_RADIO, "/dev/block/sda8" });
    devices->push_back(device);
}

static void addGalaxyJSeriesPhones(std::vector<Device *> *devices)
{
    Device *device;

    // Samsung Galaxy J7 (Exynos)
    device = new Device();
    device->setId("j7e");
    device->setCodenames({
        // LTE variant
        "j7elte", "j7eltexx",
        // 3G variant
        "j7e3g", "j7e3gxx" });
    device->setName("Samsung Galaxy J7 (Exynos)");
    device->setArchitecture(ARCH_ARM64_V8A);
    device->setBlockDevBaseDirs({ DWMMC0_13540000_BASE_DIR });
    device->setSystemBlockDevs({ DWMMC0_13540000_SYSTEM,
        "/dev/block/mmcblk0p20" });
    device->setCacheBlockDevs({ DWMMC0_13540000_CACHE,
        "/dev/block/mmcblk0p21" });
    device->setDataBlockDevs({ DWMMC0_13540000_USERDATA,
        "/dev/block/mmcblk0p23" });
    device->setBootBlockDevs({ DWMMC0_13540000_BOOT,
        "/dev/block/mmcblk0p10" });
    device->setRecoveryBlockDevs({ DWMMC0_13540000_RECOVERY,
        "/dev/block/mmcblk0p11" });
    device->setExtraBlockDevs({ DWMMC0_13540000_CDMA_RADIO,
        DWMMC0_13540000_RADIO });
    devices->push_back(device);
}

static void addGalaxyAceSeriesPhones(std::vector<Device *> *devices)
{
    Device *device;

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
    devices->push_back(device);

    // Samsung Galaxy V (Ace 4 Lite)
    device = new Device();
    device->setId("vivalto3g");
    device->setCodenames({ "vivalto3g", "vivalto3gvn", "vivalto3gub"});
    device->setName("Samsung Galaxy V (Ace 4 Lite)");
    device->setSystemBlockDevs({ "/dev/block/mmcblk0p18" });
    device->setCacheBlockDevs({ "/dev/block/mmcblk0p17" });
    device->setDataBlockDevs({ "/dev/block/mmcblk0p20" });
    device->setBootBlockDevs({ "/dev/block/mmcblk0p11" });
    device->setRecoveryBlockDevs({ "/dev/block/mmcblk0p12" });
    devices->push_back(device);
}

static void addGalaxyMegaSeriesPhones(std::vector<Device *> *devices)
{
    Device *device;

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
    devices->push_back(device);

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
    devices->push_back(device);
}

static void addGalaxyNoteSeriesTablets(std::vector<Device *> *devices)
{
    Device *device;

    // Samsung Galaxy Note 8.0 (Wifi/3G)
    device = new Device();
    device->setId("kona");
    device->setCodenames({
        // Wifi variant
        "konawifi", "konawifixx",
        // 3G variant
        "kona3g", "kona3gxx" });
    device->setName("Samsung Galaxy Note 8.0 (Wifi/3G)");
    device->setBlockDevBaseDirs({ DWMMC_BASE_DIR });
    device->setSystemBlockDevs({ DWMMC_SYSTEM, "/dev/block/mmcblk0p9" });
    device->setCacheBlockDevs({ DWMMC_CACHE, "/dev/block/mmcblk0p8" });
    device->setDataBlockDevs({ DWMMC_USERDATA, "/dev/block/mmcblk0p12" });
    device->setBootBlockDevs({ DWMMC_BOOT, "/dev/block/mmcblk0p5" });
    device->setRecoveryBlockDevs({ DWMMC_RECOVERY, "/dev/block/mmcblk0p6" });
    device->setExtraBlockDevs({ DWMMC_RADIO, "/dev/block/mmcblk0p7" });
    devices->push_back(device);

    // Samsung Galaxy Note 10.1
    device = new Device();
    device->setId("p4noterf");
    device->setCodenames({ "p4noterf", "p4noterfxx" });
    device->setName("Samsung Galaxy Note 10.1");
    device->setBlockDevBaseDirs({ DWMMC_BASE_DIR });
    device->setSystemBlockDevs({ DWMMC_SYSTEM, "/dev/block/mmcblk0p9" });
    device->setCacheBlockDevs({ DWMMC_CACHE, "/dev/block/mmcblk0p8" });
    device->setDataBlockDevs({ DWMMC_USERDATA, "/dev/block/mmcblk0p12" });
    device->setBootBlockDevs({ DWMMC_BOOT, "/dev/block/mmcblk0p5" });
    device->setRecoveryBlockDevs({ DWMMC_RECOVERY, "/dev/block/mmcblk0p6" });
    device->setExtraBlockDevs({ DWMMC_RADIO });
    devices->push_back(device);

    // Samsung Galaxy Note 10.1 Wifi (2014 Edition)
    device = new Device();
    device->setId("lt03wifi");
    device->setCodenames({ "lt03wifi", "lt03wifiue" });
    device->setName("Samsung Galaxy Note 10.1 Wifi (2014 Edition)");
    device->setBlockDevBaseDirs({ DWMMC0_BASE_DIR });
    device->setSystemBlockDevs({ DWMMC0_SYSTEM, "/dev/block/mmcblk0p20" });
    device->setCacheBlockDevs({ DWMMC0_CACHE, "/dev/block/mmcblk0p19" });
    device->setDataBlockDevs({ DWMMC0_USERDATA, "/dev/block/mmcblk0p21" });
    device->setBootBlockDevs({ DWMMC0_BOOT, "/dev/block/mmcblk0p9" });
    device->setRecoveryBlockDevs({ DWMMC0_RECOVERY, "/dev/block/mmcblk0p10" });
    device->setExtraBlockDevs({ DWMMC0_RADIO, DWMMC0_CDMA_RADIO });
    devices->push_back(device);

    // Samsung Galaxy Note 10.1 3G (2014 Edition)
    device = new Device();
    device->setId("lt033g");
    device->setCodenames({ "lt033g", "lt033gxx" });
    device->setName("Samsung Galaxy Note 10.1 3G (2014 Edition)");
    device->setBlockDevBaseDirs({ DWMMC0_BASE_DIR });
    device->setSystemBlockDevs({ DWMMC0_SYSTEM, "/dev/block/mmcblk0p20" });
    device->setCacheBlockDevs({ DWMMC0_CACHE, "/dev/block/mmcblk0p19" });
    device->setDataBlockDevs({ DWMMC0_USERDATA, "/dev/block/mmcblk0p21" });
    device->setBootBlockDevs({ DWMMC0_BOOT, "/dev/block/mmcblk0p9" });
    device->setRecoveryBlockDevs({ DWMMC0_RECOVERY, "/dev/block/mmcblk0p10" });
    device->setExtraBlockDevs({ DWMMC0_RADIO, DWMMC0_CDMA_RADIO });
    devices->push_back(device);

    // Samsung Galaxy Note 10.1 LTE (2014 Edition)
    device = new Device();
    device->setId("lt03lte");
    device->setCodenames({ "lt03lte", "lt03ltexx" });
    device->setName("Samsung Galaxy Note 10.1 LTE (2014 Edition)");
    device->setBlockDevBaseDirs({ QCOM_BASE_DIR });
    device->setSystemBlockDevs({ QCOM_SYSTEM, "/dev/block/mmcblk0p23" });
    device->setCacheBlockDevs({ QCOM_CACHE, "/dev/block/mmcblk0p24" });
    device->setDataBlockDevs({ QCOM_USERDATA, "/dev/block/mmcblk0p26" });
    device->setBootBlockDevs({ QCOM_BOOT, "/dev/block/mmcblk0p14" });
    device->setRecoveryBlockDevs({ QCOM_RECOVERY, "/dev/block/mmcblk0p15" });
    devices->push_back(device);
}

static void addGalaxyTabSeriesTablets(std::vector<Device *> *devices)
{
    Device *device;

    // Samsung Galaxy Tab 2 7.0/10.1 (Wifi/3G)
    device = new Device();
    device->setId("espresso10");
    device->setCodenames({
        // 7.0" Wifi variant
        "espressowifi", "espressowifixx",
        // 7.0" 3G variant
        "espressorf", "espressorfxx",
        // 10.1" Wifi variant
        "espresso10wifi", "espresso10wifixx",
        // 10.1" 3G variant
        "espresso10rf", "espresso10rfxx", "espresso3g" });
    device->setName("Samsung Galaxy Tab 2 7.0/10.1 (Wifi/3G)");
    device->setBlockDevBaseDirs({ OMAP_BASE_DIR });
    device->setSystemBlockDevs({ OMAP_FACTORYFS, "/dev/block/mmcblk0p9" });
    device->setCacheBlockDevs({ OMAP_CACHE, "/dev/block/mmcblk0p7" });
    device->setDataBlockDevs({ OMAP_DATAFS, "/dev/block/mmcblk0p10" });
    device->setBootBlockDevs({ OMAP_KERNEL, "/dev/block/mmcblk0p5" });
    device->setRecoveryBlockDevs({ OMAP_RECOVERY, "/dev/block/mmcblk0p6" });
    devices->push_back(device);

    // Samsung Galaxy Tab 3 8.0 (Wifi)
    device = new Device();
    device->setId("lt01wifi");
    device->setCodenames({ "lt01wifi", "lt01wifixx" });
    device->setName("Samsung Galaxy Tab 3 8.0 (Wifi)");
    device->setBlockDevBaseDirs({ DWMMC_BASE_DIR });
    device->setSystemBlockDevs({ DWMMC_SYSTEM, "/dev/block/mmcblk0p20" });
    device->setCacheBlockDevs({ DWMMC_CACHE, "/dev/block/mmcblk0p19" });
    device->setDataBlockDevs({ DWMMC_USERDATA, "/dev/block/mmcblk0p21" });
    device->setBootBlockDevs({ DWMMC_BOOT, "/dev/block/mmcblk0p9" });
    device->setRecoveryBlockDevs({ DWMMC_RECOVERY, "/dev/block/mmcblk0p10" });
    devices->push_back(device);

    // Samsung Galaxy Tab 3 10.1
    device = new Device();
    device->setId("santos");
    device->setCodenames({
        // 3G variant
        "santos103g", "santos103gxx",
        // LTE variant
        "santos10lte", "santos10ltexx",
        // Wifi variant
        "santos10wifi", "santos10wifixx" });
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
    devices->push_back(device);

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
    devices->push_back(device);

    // Samsung Galaxy Tab Pro 8.4 (Wifi)
    device = new Device();
    device->setId("mondrianwifi");
    device->setCodenames({ "mondrianwifi", "mondrianwifiue",
        "mondrianwifixx" });
    device->setName("Samsung Galaxy Tab Pro 8.4 (Wifi)");
    device->setBlockDevBaseDirs({ QCOM_BASE_DIR });
    device->setSystemBlockDevs({ QCOM_SYSTEM, "/dev/block/mmcblk0p23" });
    device->setCacheBlockDevs({ QCOM_CACHE, "/dev/block/mmcblk0p24" });
    device->setDataBlockDevs({ QCOM_USERDATA, "/dev/block/mmcblk0p26" });
    device->setBootBlockDevs({ QCOM_BOOT, "/dev/block/mmcblk0p14" });
    device->setRecoveryBlockDevs({ QCOM_RECOVERY, "/dev/block/mmcblk0p15" });
    devices->push_back(device);

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
    devices->push_back(device);

    // Samsung Galaxy Tab Pro 10.1 (LTE)
    device = new Device();
    device->setId("picassolte");
    device->setCodenames({ "picassolte", "picassoltexx" });
    device->setName("Samsung Galaxy Tab Pro 10.1 (LTE)");
    device->setBlockDevBaseDirs({ QCOM_BASE_DIR });
    device->setSystemBlockDevs({ QCOM_SYSTEM, "/dev/block/mmcblk0p23" });
    device->setCacheBlockDevs({ QCOM_CACHE, "/dev/block/mmcblk0p24" });
    device->setDataBlockDevs({ QCOM_USERDATA, "/dev/block/mmcblk0p26" });
    device->setBootBlockDevs({ QCOM_BOOT, "/dev/block/mmcblk0p14" });
    device->setRecoveryBlockDevs({ QCOM_RECOVERY, "/dev/block/mmcblk0p15" });
    devices->push_back(device);

    // Samsung Galaxy Tab S 8.4/10.5
    device = new Device();
    device->setId("tab_s");
    device->setCodenames({
        // 8.4" variant (wifi)
        "klimtwifi", "klimtwifikx",
        // 8.4" variant (LTE)
        "klimtlte", "klimtltexx",
        // 10.5" variant (wifi)
        "chagallwifi", "chagallwifixx",
        // 10.5" variant (LTE)
        "chagalllte", "chagallltexx" });
    device->setName("Samsung Galaxy Tab S 8.4/10.5");
    device->setBlockDevBaseDirs({ DWMMC0_BASE_DIR });
    device->setSystemBlockDevs({ DWMMC0_SYSTEM, "/dev/block/mmcblk0p18" });
    device->setCacheBlockDevs({ DWMMC0_CACHE, "/dev/block/mmcblk0p19" });
    device->setDataBlockDevs({ DWMMC0_USERDATA, "/dev/block/mmcblk0p21" });
    device->setBootBlockDevs({ DWMMC0_BOOT, "/dev/block/mmcblk0p9" });
    device->setRecoveryBlockDevs({ DWMMC0_RECOVERY, "/dev/block/mmcblk0p10" });
    device->setExtraBlockDevs({ DWMMC0_RADIO, DWMMC0_CDMA_RADIO });
    devices->push_back(device);

    // Samsung Galaxy Tab S2 8.0/9.7 (Wifi)
    device = new Device();
    device->setId("tab_s2_wifi");
    device->setCodenames({
        // 8.0" variant
        "gts28wifi", "gts28wifixx",
        // 9.7" variant
        "gts210wifi", "gts210wifixx" });
    device->setName("Samsung Galaxy Tab S2 8.0/9.7 (Wifi)");
    device->setBlockDevBaseDirs({ DWMMC0_15540000_BASE_DIR });
    device->setSystemBlockDevs({ DWMMC0_15540000_SYSTEM,
        "/dev/block/mmcblk0p19" });
    device->setCacheBlockDevs({ DWMMC0_15540000_CACHE,
        "/dev/block/mmcblk0p20" });
    device->setDataBlockDevs({ DWMMC0_15540000_USERDATA,
        "/dev/block/mmcblk0p22" });
    device->setBootBlockDevs({ DWMMC0_15540000_BOOT,
        "/dev/block/mmcblk0p9" });
    device->setRecoveryBlockDevs({ DWMMC0_15540000_RECOVERY,
        "/dev/block/mmcblk0p10" });
    device->setExtraBlockDevs({ DWMMC0_15540000_RADIO,
        DWMMC0_15540000_CDMA_RADIO });
    devices->push_back(device);
}

static void addOtherGalaxySeries(std::vector<Device *> *devices)
{
    Device *device;

    // Samsung Galaxy Grand 2 (Qcom)
    device = new Device();
    device->setId("ms013g");
    device->setCodenames({ "ms013g", "ms01lte" });
    device->setName("Samsung Galaxy Grand 2 (Qcom)");
    device->setBlockDevBaseDirs({ QCOM_BASE_DIR });
    device->setSystemBlockDevs({ QCOM_SYSTEM, "/dev/block/mmcblk0p22" });
    device->setCacheBlockDevs({ QCOM_CACHE, "/dev/block/mmcblk0p23" });
    device->setDataBlockDevs({ QCOM_USERDATA, "/dev/block/mmcblk0p25" });
    device->setBootBlockDevs({ QCOM_BOOT, "/dev/block/mmcblk0p14" });
    device->setRecoveryBlockDevs({ QCOM_RECOVERY, "/dev/block/mmcblk0p15" });
    devices->push_back(device);

    // Samsung Galaxy Star
    device = new Device();
    device->setId("mint");
    device->setCodenames({ "mint", "mint2g"});
    device->setName("Samsung Galaxy Star");
    device->setSystemBlockDevs({ "/dev/block/mmcblk0p21" });
    device->setCacheBlockDevs({ "/dev/block/mmcblk0p20" });
    device->setDataBlockDevs({ "/dev/block/mmcblk0p25" });
    device->setBootBlockDevs({ "/dev/block/mmcblk0p5" });
    device->setRecoveryBlockDevs({ "/dev/block/mmcblk0p6" });
    devices->push_back(device);

    // Samsung Galaxy Alpha (Exynos)
    device = new Device();
    device->setId("slte");
    device->setCodenames({
        // G850F
        "slte", "sltexx",
        // G850S/K/L
        "slteskt", "sltektt", "sltelgu" });
    device->setName("Samsung Galaxy Alpha (Exynos)");
    device->setBlockDevBaseDirs({ DWMMC0_15540000_BASE_DIR });
    device->setSystemBlockDevs({ DWMMC0_15540000_SYSTEM,
        "/dev/block/mmcblk0p18" });
    device->setCacheBlockDevs({ DWMMC0_15540000_CACHE,
        "/dev/block/mmcblk0p19" });
    device->setDataBlockDevs({ DWMMC0_15540000_USERDATA,
        "/dev/block/mmcblk0p21" });
    device->setBootBlockDevs({ DWMMC0_15540000_BOOT,
        "/dev/block/mmcblk0p9" });
    device->setRecoveryBlockDevs({ DWMMC0_15540000_RECOVERY,
        "/dev/block/mmcblk0p10" });
    device->setExtraBlockDevs({ DWMMC0_15540000_RADIO,
        DWMMC0_15540000_CDMA_RADIO });
    devices->push_back(device);
}

void addSamsungDevices(std::vector<Device *> *devices)
{
    // Galaxy Phones
    addGalaxySSeriesPhones(devices);
    addGalaxyNoteSeriesPhones(devices);
    addGalaxyJSeriesPhones(devices);
    addGalaxyAceSeriesPhones(devices);
    addGalaxyMegaSeriesPhones(devices);
    // Galaxy Tablets
    addGalaxyTabSeriesTablets(devices);
    addGalaxyNoteSeriesTablets(devices);
    // Other Galaxy Series
    addOtherGalaxySeries(devices);
}

}
