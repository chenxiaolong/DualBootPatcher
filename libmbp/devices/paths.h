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

#pragma once

// Used by some Intel SOCs that do not expose a platform name in /sys
#define BLOCK_BASE_DIR          "/dev/block/by-name"
#define BLOCK_BOOT              BLOCK_BASE_DIR "/boot"
#define BLOCK_CACHE             BLOCK_BASE_DIR "/cache"
#define BLOCK_DATA              BLOCK_BASE_DIR "/data"
#define BLOCK_RECOVERY          BLOCK_BASE_DIR "/recovery"
#define BLOCK_SYSTEM            BLOCK_BASE_DIR "/system"

// Used by devices where the 'androidboot.bootdevice' kernel cmdline
// parameter is specified either in the ramdisk or by the bootloader. It is
// simply a symlink to the device's block device platform directory.
#define BOOTDEVICE_BASE_DIR     "/dev/block/bootdevice/by-name"
#define BOOTDEVICE_BOOT         BOOTDEVICE_BASE_DIR "/boot"
#define BOOTDEVICE_CACHE        BOOTDEVICE_BASE_DIR "/cache"
#define BOOTDEVICE_RECOVERY     BOOTDEVICE_BASE_DIR "/recovery"
#define BOOTDEVICE_SYSTEM       BOOTDEVICE_BASE_DIR "/system"
#define BOOTDEVICE_USERDATA     BOOTDEVICE_BASE_DIR "/userdata"

// Block device paths used by Qualcomm SOCs (Snapdragon 805 and lower)
#define QCOM_BASE_DIR           "/dev/block/platform/msm_sdcc.1/by-name"
#define QCOM_ABOOT              QCOM_BASE_DIR "/aboot"
#define QCOM_BOOT               QCOM_BASE_DIR "/boot"
#define QCOM_CACHE              QCOM_BASE_DIR "/cache"
#define QCOM_DBI                QCOM_BASE_DIR "/dbi"
#define QCOM_IMGDATA            QCOM_BASE_DIR "/imgdata"
#define QCOM_LOGO               QCOM_BASE_DIR "/LOGO"
#define QCOM_MISC               QCOM_BASE_DIR "/misc"
#define QCOM_MODEM              QCOM_BASE_DIR "/modem"
#define QCOM_OPPOSTANVBK        QCOM_BASE_DIR "/oppostanvbk"
#define QCOM_RECOVERY           QCOM_BASE_DIR "/recovery"
#define QCOM_FOTA_RECOVERY      QCOM_BASE_DIR "/FOTAKernel"
#define QCOM_RESERVE4           QCOM_BASE_DIR "/reserve4"
#define QCOM_RPM                QCOM_BASE_DIR "/rpm"
#define QCOM_SBL1               QCOM_BASE_DIR "/sbl1"
#define QCOM_SDI                QCOM_BASE_DIR "/sdi"
#define QCOM_SYSTEM             QCOM_BASE_DIR "/system"
#define QCOM_TZ                 QCOM_BASE_DIR "/tz"
#define QCOM_USERDATA           QCOM_BASE_DIR "/userdata"

// Block device paths used by the Snapdragon 808 and 810 in Android < 6.0
#define F9824900_BASE_DIR       "/dev/block/platform/f9824900.sdhci/by-name"
#define F9824900_BOOT           F9824900_BASE_DIR "/boot"
#define F9824900_CACHE          F9824900_BASE_DIR "/cache"
#define F9824900_RECOVERY       F9824900_BASE_DIR "/recovery"
#define F9824900_SYSTEM         F9824900_BASE_DIR "/system"
#define F9824900_USERDATA       F9824900_BASE_DIR "/userdata"

// Block device paths used by the Snapdragon 808 and 810 in Android >= 6.0
#define F9824900_SOC0_BASE_DIR  "/dev/block/platform/soc.0/f9824900.sdhci/by-name"
#define F9824900_SOC0_BOOT      F9824900_SOC0_BASE_DIR "/boot"
#define F9824900_SOC0_CACHE     F9824900_SOC0_BASE_DIR "/cache"
#define F9824900_SOC0_RECOVERY  F9824900_SOC0_BASE_DIR "/recovery"
#define F9824900_SOC0_SYSTEM    F9824900_SOC0_BASE_DIR "/system"
#define F9824900_SOC0_USERDATA  F9824900_SOC0_BASE_DIR "/userdata"

// Block device paths used by the Exynos 7420
#define UFS_BASE_DIR            "/dev/block/platform/15570000.ufs/by-name"
#define UFS_BOOT                UFS_BASE_DIR "/BOOT"
#define UFS_CACHE               UFS_BASE_DIR "/CACHE"
#define UFS_RADIO               UFS_BASE_DIR "/RADIO"
#define UFS_RECOVERY            UFS_BASE_DIR "/RECOVERY"
#define UFS_SYSTEM              UFS_BASE_DIR "/SYSTEM"
#define UFS_USERDATA            UFS_BASE_DIR "/USERDATA"

// Block device paths used by the Exynos 5422
#define DWMMC0_12200000_BASE_DIR   "/dev/block/platform/12200000.dwmmc0/by-name"
#define DWMMC0_12200000_BOOT       DWMMC0_12200000_BASE_DIR "/BOOT"
#define DWMMC0_12200000_CACHE      DWMMC0_12200000_BASE_DIR "/CACHE"
#define DWMMC0_12200000_CDMA_RADIO DWMMC0_12200000_BASE_DIR "/CDMA-RADIO"
#define DWMMC0_12200000_RADIO      DWMMC0_12200000_BASE_DIR "/RADIO"
#define DWMMC0_12200000_RECOVERY   DWMMC0_12200000_BASE_DIR "/RECOVERY"
#define DWMMC0_12200000_SYSTEM     DWMMC0_12200000_BASE_DIR "/SYSTEM"
#define DWMMC0_12200000_USERDATA   DWMMC0_12200000_BASE_DIR "/USERDATA"

// Block device paths used by the Exynos 5430/5433
#define DWMMC0_15540000_BASE_DIR   "/dev/block/platform/15540000.dwmmc0/by-name"
#define DWMMC0_15540000_BOOT       DWMMC0_15540000_BASE_DIR "/BOOT"
#define DWMMC0_15540000_CACHE      DWMMC0_15540000_BASE_DIR "/CACHE"
#define DWMMC0_15540000_CDMA_RADIO DWMMC0_15540000_BASE_DIR "/CDMA-RADIO"
#define DWMMC0_15540000_RADIO      DWMMC0_15540000_BASE_DIR "/RADIO"
#define DWMMC0_15540000_RECOVERY   DWMMC0_15540000_BASE_DIR "/RECOVERY"
#define DWMMC0_15540000_SYSTEM     DWMMC0_15540000_BASE_DIR "/SYSTEM"
#define DWMMC0_15540000_USERDATA   DWMMC0_15540000_BASE_DIR "/USERDATA"

// Block device paths used by the Exynos 4412 Quad
#define DWMMC_BASE_DIR          "/dev/block/platform/dw_mmc/by-name"
#define DWMMC_BOOT              DWMMC_BASE_DIR "/BOOT"
#define DWMMC_CACHE             DWMMC_BASE_DIR "/CACHE"
#define DWMMC_RADIO             DWMMC_BASE_DIR "/RADIO"
#define DWMMC_RECOVERY          DWMMC_BASE_DIR "/RECOVERY"
#define DWMMC_SYSTEM            DWMMC_BASE_DIR "/SYSTEM"
#define DWMMC_USERDATA          DWMMC_BASE_DIR "/USERDATA"

// Block device paths used by the Exynos 5420
#define DWMMC0_BASE_DIR         "/dev/block/platform/dw_mmc.0/by-name"
#define DWMMC0_BOOT             DWMMC0_BASE_DIR "/BOOT"
#define DWMMC0_CACHE            DWMMC0_BASE_DIR "/CACHE"
#define DWMMC0_CDMA_RADIO       DWMMC0_BASE_DIR "/CDMA-RADIO"
#define DWMMC0_RADIO            DWMMC0_BASE_DIR "/RADIO"
#define DWMMC0_RECOVERY         DWMMC0_BASE_DIR "/RECOVERY"
#define DWMMC0_SYSTEM           DWMMC0_BASE_DIR "/SYSTEM"
#define DWMMC0_USERDATA         DWMMC0_BASE_DIR "/USERDATA"

// Block device paths used by Mediatek devices
#define MTK_BASE_DIR            "/dev/block/platform/mtk-msdc.0/by-name"
#define MTK_BOOT                MTK_BASE_DIR "/boot"
#define MTK_CACHE               MTK_BASE_DIR "/cache"
#define MTK_LOGO                MTK_BASE_DIR "/logo"
#define MTK_PARA                MTK_BASE_DIR "/para"
#define MTK_RECOVERY            MTK_BASE_DIR "/recovery"
#define MTK_SYSTEM              MTK_BASE_DIR "/system"
#define MTK_TEE1                MTK_BASE_DIR "/tee1"
#define MTK_UBOOT               MTK_BASE_DIR "/uboot"
#define MTK_USERDATA            MTK_BASE_DIR "/userdata"

// Block device paths used by devices with Tegra 3 SOCs
#define TEGRA3_BASE_DIR         "/dev/block/platform/sdhci-tegra.3/by-name"
#define TEGRA3_BOOT             TEGRA3_BASE_DIR "/LNX"
#define TEGRA3_CACHE            TEGRA3_BASE_DIR "/CAC"
#define TEGRA3_RECOVERY         TEGRA3_BASE_DIR "/SOS"
#define TEGRA3_SYSTEM           TEGRA3_BASE_DIR "/APP"
#define TEGRA3_USERDATA         TEGRA3_BASE_DIR "/UDA"

// Block device paths used by devices with Hisilicon SOCs
#define HISILICON_BASE_DIR      "/dev/block/platform/hi_mci.0/by-name"
#define HISILICON_BOOT          HISILICON_BASE_DIR "/boot"
#define HISILICON_CACHE         HISILICON_BASE_DIR "/cache"
#define HISILICON_RECOVERY      HISILICON_BASE_DIR "/recovery"
#define HISILICON_SYSTEM        HISILICON_BASE_DIR "/system"
#define HISILICON_USERDATA      HISILICON_BASE_DIR "/userdata"

// Block device paths used by Intel's PCI platform. Yay for inconsistency!
#define INTEL_PCI_BASE_DIR      "/dev/block/pci/pci0000:00/0000:00:01.0/by-name"
#define INTEL_PCI_BOOT          INTEL_PCI_BASE_DIR "/BOOT"
#define INTEL_PCI_BOOT_2        INTEL_PCI_BASE_DIR "/boot"
#define INTEL_PCI_CACHE         INTEL_PCI_BASE_DIR "/CACHE"
#define INTEL_PCI_CACHE_2       INTEL_PCI_BASE_DIR "/cache"
#define INTEL_PCI_DATA_2        INTEL_PCI_BASE_DIR "/data"
#define INTEL_PCI_RADIO         INTEL_PCI_BASE_DIR "/RADIO"
#define INTEL_PCI_RECOVERY      INTEL_PCI_BASE_DIR "/RECOVERY"
#define INTEL_PCI_RECOVERY_2    INTEL_PCI_BASE_DIR "/recovery"
#define INTEL_PCI_SYSTEM        INTEL_PCI_BASE_DIR "/SYSTEM"
#define INTEL_PCI_SYSTEM_2      INTEL_PCI_BASE_DIR "/system"
#define INTEL_PCI_USERDATA      INTEL_PCI_BASE_DIR "/USERDATA"
