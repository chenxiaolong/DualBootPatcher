/*
 * Copyright (C) 2014  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "cwrapper/cbootimage.h"

#include <cassert>

#include "cwrapper/private/util.h"

#include "bootimage.h"


#define CAST(x) \
    assert(x != nullptr); \
    mbp::BootImage *bi = reinterpret_cast<mbp::BootImage *>(x);
#define CCAST(x) \
    assert(x != nullptr); \
    const mbp::BootImage *bi = reinterpret_cast<const mbp::BootImage *>(x);


/*!
 * \file cbootimage.h
 * \brief C Wrapper for BootImage
 *
 * Please see the documentation for BootImage from the C++ API for more details.
 * The C functions directly correspond to the BootImage member functions.
 *
 * \sa BootImage
 */

extern "C" {

/*!
 * \brief Create a new CBootImage object.
 *
 * \note The returned object must be freed with mbp_bootimage_destroy().
 *
 * \return New CBootImage
 */
CBootImage * mbp_bootimage_create(void)
{
    return reinterpret_cast<CBootImage *>(new mbp::BootImage());
}

/*!
 * \brief Destroys a CBootImage object.
 *
 * \param bootImage CBootImage to destroy
 */
void mbp_bootimage_destroy(CBootImage *bootImage)
{
    CAST(bootImage);
    delete bi;
}

/*!
 * \brief Get the error information
 *
 * \note The returned CPatcherError is filled with valid data only if a
 *       CBootImage operation has failed.
 *
 * \note The returned CPatcherError should be freed with mbp_error_destroy()
 *       when it is no longer needed.
 *
 * \param bootImage CBootImage object
 *
 * \return CPatcherError
 *
 * \sa BootImage::error()
 */
CPatcherError * mbp_bootimage_error(const CBootImage *bootImage)
{
    CCAST(bootImage);
    mbp::PatcherError *pe = new mbp::PatcherError(bi->error());
    return reinterpret_cast<CPatcherError *>(pe);
}

/*!
 * \brief Load boot image from binary data
 *
 * \param bootImage CBootImage object
 * \param data Byte array containing binary data
 * \param size Size of byte array
 *
 * \return true on success or false on failure and error set appropriately
 *
 * \sa BootImage::load(const std::vector<unsigned char> &)
 */
bool mbp_bootimage_load_data(CBootImage *bootImage,
                             const unsigned char *data, size_t size)
{
    CAST(bootImage);
    return bi->load(data, size);
}

/*!
 * \brief Load boot image from a file
 *
 * \param bootImage CBootImage object
 * \param filename Path to boot image file
 *
 * \return true on success or false on failure and error set appropriately
 *
 * \sa BootImage::load(const std::string &)
 */
bool mbp_bootimage_load_file(CBootImage *bootImage,
                             const char *filename)
{
    CAST(bootImage);
    return bi->loadFile(filename);
}

/*!
 * \brief Constructs the boot image binary data
 *
 * \note The output data is dynamically allocated. It should be free()'d
 *       when it is no longer needed.
 *
 * \param bootImage CBootImage object
 * \param data Output data
 * \param size Size of output data
 *
 * \sa BootImage::create()
 */
bool mbp_bootimage_create_data(const CBootImage *bootImage,
                               unsigned char **data, size_t *size)
{
    CCAST(bootImage);
    std::vector<unsigned char> vData;
    if (bi->create(&vData)) {
        vector_to_data(vData, reinterpret_cast<void **>(data), size);
        return true;
    } else {
        return false;
    }
}

/*!
 * \brief Constructs boot image and writes it to a file
 *
 * \param bootImage CBootImage object
 * \param filename Path to output file
 *
 * \return true on success or false on failure and error set appropriately
 *
 * \sa BootImage::createFile()
 */
bool mbp_bootimage_create_file(CBootImage *bootImage,
                               const char *filename)
{
    CAST(bootImage);
    return bi->createFile(filename);
}

enum BootImageType mbp_bootimage_was_type(const CBootImage *bootImage)
{
    CCAST(bootImage);
    return static_cast<BootImageType>(bi->wasType());
}

void mbp_bootimage_set_type(CBootImage *bootImage, enum BootImageType type)
{
    CAST(bootImage);
    bi->setType(static_cast<mbp::BootImage::Type>(type));
}

/*!
 * \brief Board name field in the boot image header
 *
 * \param bootImage CBootImage object
 *
 * \return Board name
 *
 * \sa BootImage::boardName()
 */
const char * mbp_bootimage_boardname(const CBootImage *bootImage)
{
    CCAST(bootImage);
    return bi->boardNameC();
}

/*!
 * \brief Set the board name field in the boot image header
 *
 * \param bootImage CBootImage object
 * \param name Board name
 *
 * \sa BootImage::setBoardName()
 */
void mbp_bootimage_set_boardname(CBootImage *bootImage,
                                 const char *name)
{
    CAST(bootImage);
    bi->setBoardNameC(name);
}

/*!
 * \brief Resets the board name field in the boot image header to the default
 *
 * \param bootImage CBootImage object
 *
 * \sa BootImage::resetBoardName()
 */
void mbp_bootimage_reset_boardname(CBootImage *bootImage)
{
    CAST(bootImage);
    bi->resetBoardName();
}

/*!
 * \brief Kernel cmdline in the boot image header
 *
 * \param bootImage CBootImage object
 *
 * \return Kernel cmdline
 *
 * \sa BootImage::kernelCmdline()
 */
const char * mbp_bootimage_kernel_cmdline(const CBootImage *bootImage)
{
    CCAST(bootImage);
    return bi->kernelCmdlineC();
}

/*!
 * \brief Set the kernel cmdline in the boot image header
 *
 * \param bootImage CBootImage object
 * \param cmdline Kernel cmdline
 *
 * \sa BootImage::setKernelCmdline()
 */
void mbp_bootimage_set_kernel_cmdline(CBootImage *bootImage,
                                      const char *cmdline)
{
    CAST(bootImage);
    bi->setKernelCmdlineC(cmdline);
}

/*!
 * \brief Resets the kernel cmdline to the default
 *
 * \param bootImage CBootImage object
 *
 * \sa BootImage::resetKernelCmdline()
 */
void mbp_bootimage_reset_kernel_cmdline(CBootImage *bootImage)
{
    CAST(bootImage);
    bi->resetKernelCmdline();
}

/*!
 * \brief Page size field in the boot image header
 *
 * \param bootImage CBootImage object
 *
 * \return Page size
 *
 * \sa BootImage::pageSize()
 */
uint32_t mbp_bootimage_page_size(const CBootImage *bootImage)
{
    CCAST(bootImage);
    return bi->pageSize();
}

/*!
 * \brief Set the page size field in the boot image header
 *
 * \param bootImage CBootImage object
 * \param pageSize Page size
 *
 * \sa BootImage::setPageSize()
 */
void mbp_bootimage_set_page_size(CBootImage *bootImage,
                                 uint32_t pageSize)
{
    CAST(bootImage);
    bi->setPageSize(pageSize);
}

/*!
 * \brief Resets the page size field in the header to the default
 *
 * \param bootImage CBootImage object
 *
 * \sa BootImage::resetPageSize()
 */
void mbp_bootimage_reset_page_size(CBootImage *bootImage)
{
    CAST(bootImage);
    bi->resetPageSize();
}

/*!
 * \brief Kernel address field in the boot image header
 *
 * \param bootImage CBootImage object
 *
 * \return Kernel address
 *
 * \sa BootImage::kernelAddress()
 */
uint32_t mbp_bootimage_kernel_address(const CBootImage *bootImage)
{
    CCAST(bootImage);
    return bi->kernelAddress();
}

/*!
 * \brief Set the kernel address field in the boot image header
 *
 * \param bootImage CBootImage object
 * \param address Kernel address
 *
 * \sa BootImage::setKernelAddress()
 */
void mbp_bootimage_set_kernel_address(CBootImage *bootImage,
                                      uint32_t address)
{
    CAST(bootImage);
    bi->setKernelAddress(address);
}

/*!
 * \brief Resets the kernel address field in the header to the default
 *
 * \param bootImage CBootImage object
 *
 * \sa BootImage::resetKernelAddress()
 */
void mbp_bootimage_reset_kernel_address(CBootImage *bootImage)
{
    CAST(bootImage);
    bi->resetKernelAddress();
}

/*!
 * \brief Ramdisk address field in the boot image header
 *
 * \param bootImage CBootImage object
 *
 * \return Ramdisk address
 *
 * \sa BootImage::ramdiskAddress()
 */
uint32_t mbp_bootimage_ramdisk_address(const CBootImage *bootImage)
{
    CCAST(bootImage);
    return bi->ramdiskAddress();
}

/*!
 * \brief Set the ramdisk address field in the boot image header
 *
 * \param bootImage CBootImage object
 * \param address Ramdisk address
 *
 * \sa BootImage::setRamdiskAddress()
 */
void mbp_bootimage_set_ramdisk_address(CBootImage *bootImage,
                                       uint32_t address)
{
    CAST(bootImage);
    bi->setRamdiskAddress(address);
}

/*!
 * \brief Resets the ramdisk address field in the header to the default
 *
 * \param bootImage CBootImage object
 *
 * \sa BootImage::resetRamdiskAddress()
 */
void mbp_bootimage_reset_ramdisk_address(CBootImage *bootImage)
{
    CAST(bootImage);
    bi->resetRamdiskAddress();
}

/*!
 * \brief Second bootloader address field in the boot image header
 *
 * \param bootImage CBootImage object
 *
 * \return Second bootloader address
 *
 * \sa BootImage::secondBootloaderAddress()
 */
uint32_t mbp_bootimage_second_bootloader_address(const CBootImage *bootImage)
{
    CCAST(bootImage);
    return bi->secondBootloaderAddress();
}

/*!
 * \brief Set the second bootloader address field in the boot image header
 *
 * \param bootImage CBootImage object
 * \param address Second bootloader address
 *
 * \sa BootImage::setSecondBootloaderAddress()
 */
void mbp_bootimage_set_second_bootloader_address(CBootImage *bootImage,
                                                 uint32_t address)
{
    CAST(bootImage);
    bi->setSecondBootloaderAddress(address);
}

/*!
 * \brief Resets the second bootloader address field in the header to the
 *        default
 *
 * \param bootImage CBootImage object
 *
 * \sa BootImage::resetSecondBootloaderAddress()
 */
void mbp_bootimage_reset_second_bootloader_address(CBootImage *bootImage)
{
    CAST(bootImage);
    bi->resetSecondBootloaderAddress();
}

/*!
 * \brief Kernel tags address field in the boot image header
 *
 * \param bootImage CBootImage object
 *
 * \return Kernel tags address
 *
 * \sa BootImage::kernelTagsAddress()
 */
uint32_t mbp_bootimage_kernel_tags_address(const CBootImage *bootImage)
{
    CCAST(bootImage);
    return bi->kernelTagsAddress();
}

/*!
 * \brief Set the kernel tags address field in the boot image header
 *
 * \param bootImage CBootImage object
 * \param address Kernel tags address
 *
 * \sa BootImage::setKernelTagsAddress()
 */
void mbp_bootimage_set_kernel_tags_address(CBootImage *bootImage,
                                           uint32_t address)
{
    CAST(bootImage);
    bi->setKernelTagsAddress(address);
}

/*!
 * \brief Resets the kernel tags address field in the header to the default
 *
 * \param bootImage CBootImage object
 *
 * \sa BootImage::resetKernelTagsAddress()
 */
void mbp_bootimage_reset_kernel_tags_address(CBootImage *bootImage)
{
    CAST(bootImage);
    bi->resetKernelTagsAddress();
}

uint32_t mbp_bootimage_ipl_address(const CBootImage *bootImage)
{
    CCAST(bootImage);
    return bi->iplAddress();
}

void mbp_bootimage_set_ipl_address(CBootImage *bootImage, uint32_t address)
{
    CAST(bootImage);
    bi->setIplAddress(address);
}

void mbp_bootimage_reset_ipl_address(CBootImage *bootImage)
{
    CAST(bootImage);
    bi->resetIplAddress();
}

uint32_t mbp_bootimage_rpm_address(const CBootImage *bootImage)
{
    CCAST(bootImage);
    return bi->rpmAddress();
}

void mbp_bootimage_set_rpm_address(CBootImage *bootImage, uint32_t address)
{
    CAST(bootImage);
    bi->setRpmAddress(address);
}

void mbp_bootimage_reset_rpm_address(CBootImage *bootImage)
{
    CAST(bootImage);
    bi->resetRpmAddress();
}

uint32_t mbp_bootimage_appsbl_address(const CBootImage *bootImage)
{
    CCAST(bootImage);
    return bi->appsblAddress();
}

void mbp_bootimage_set_appsbl_address(CBootImage *bootImage, uint32_t address)
{
    CAST(bootImage);
    bi->setAppsblAddress(address);
}

void mbp_bootimage_reset_appsbl_address(CBootImage *bootImage)
{
    CAST(bootImage);
    bi->resetAppsblAddress();
}

uint32_t mbp_bootimage_entrypoint_address(const CBootImage *bootImage)
{
    CCAST(bootImage);
    return bi->entrypointAddress();
}

void mbp_bootimage_set_entrypoint_address(CBootImage *bootImage,
                                          uint32_t address)
{
    CAST(bootImage);
    bi->setEntrypointAddress(address);
}

void mbp_bootimage_reset_entrypoint_address(CBootImage *bootImage)
{
    CAST(bootImage);
    bi->resetEntrypointAddress();
}

/*!
 * \brief Set all of the addresses using offsets and a base address
 *
 * \param bootImage CBootImage object
 * \param base Base address
 * \param kernelOffset Kernel offset
 * \param ramdiskOffset Ramdisk offset
 * \param secondBootloaderOffset Second bootloader offset
 * \param kernelTagsOffset Kernel tags offset
 *
 * \sa BootImage::setAddresses()
 */
void mbp_bootimage_set_addresses(CBootImage *bootImage,
                                 uint32_t base,
                                 uint32_t kernelOffset,
                                 uint32_t ramdiskOffset,
                                 uint32_t secondBootloaderOffset,
                                 uint32_t kernelTagsOffset)
{
    CAST(bootImage);
    bi->setAddresses(base, kernelOffset, ramdiskOffset,
                     secondBootloaderOffset, kernelTagsOffset);
}

/*!
 * \brief Kernel image
 *
 * \param bootImage CBootImage object
 * \param data Output data
 * \param size Output size
 *
 * \sa BootImage::kernelImage()
 */
void mbp_bootimage_kernel_image(const CBootImage *bootImage,
                                const unsigned char **data, size_t *size)
{
    CCAST(bootImage);
    bi->kernelImageC(data, size);
}

/*!
 * \brief Set the kernel image
 *
 * \param bootImage CBootImage object
 * \param data Byte array containing kernel image
 * \param size Size of byte array
 *
 * \sa BootImage::setKernelImage()
 */
void mbp_bootimage_set_kernel_image(CBootImage *bootImage,
                                    const unsigned char *data, size_t size)
{
    CAST(bootImage);
    bi->setKernelImageC(data, size);
}

/*!
 * \brief Ramdisk image
 *
 * \param bootImage CBootImage object
 * \param data Output data
 * \param size Output size
 *
 * \sa BootImage::ramdiskImage()
 */
void mbp_bootimage_ramdisk_image(const CBootImage *bootImage,
                                 const unsigned char **data, size_t *size)
{
    CCAST(bootImage);
    bi->ramdiskImageC(data, size);
}

/*!
 * \brief Set the ramdisk image
 *
 * \param bootImage CBootImage object
 * \param data Byte array containing ramdisk image
 * \param size Size of byte array
 *
 * \sa BootImage::setRamdiskImage()
 */
void mbp_bootimage_set_ramdisk_image(CBootImage *bootImage,
                                     const unsigned char *data, size_t size)
{
    CAST(bootImage);
    bi->setRamdiskImageC(data, size);
}

/*!
 * \brief Second bootloader image
 *
 * \param bootImage CBootImage object
 * \param data Output data
 * \param size Output size
 *
 * \sa BootImage::secondBootloaderImage()
 */
void mbp_bootimage_second_bootloader_image(const CBootImage *bootImage,
                                           const unsigned char **data, size_t *size)
{
    CCAST(bootImage);
    bi->secondBootloaderImageC(data, size);
}

/*!
 * \brief Set the second bootloader image
 *
 * \param bootImage CBootImage object
 * \param data Byte array containing second bootloader image
 * \param size Size of byte array
 *
 * \sa BootImage::setSecondBootloaderImage()
 */
void mbp_bootimage_set_second_bootloader_image(CBootImage *bootImage,
                                               const unsigned char *data, size_t size)
{
    CAST(bootImage);
    bi->setSecondBootloaderImageC(data, size);
}

/*!
 * \brief Device tree image
 *
 * \param bootImage CBootImage object
 * \param data Output data
 * \param size Output size
 *
 * \sa BootImage::deviceTreeImage()
 */
void mbp_bootimage_device_tree_image(const CBootImage *bootImage,
                                     const unsigned char **data, size_t *size)
{
    CCAST(bootImage);
    bi->deviceTreeImageC(data, size);
}

/*!
 * \brief Set the device tree image
 *
 * \param bootImage CBootImage object
 * \param data Byte array containing device tree image
 * \param size Size of byte array
 *
 * \sa BootImage::setDeviceTreeImage()
 */
void mbp_bootimage_set_device_tree_image(CBootImage *bootImage,
                                         const unsigned char *data, size_t size)
{
    CAST(bootImage);
    bi->setDeviceTreeImageC(data, size);
}

void mbp_bootimage_aboot_image(const CBootImage *bootImage,
                               const unsigned char **data, size_t *size)
{
    CCAST(bootImage);
    bi->abootImageC(data, size);
}

void mbp_bootimage_set_aboot_image(CBootImage *bootImage,
                                   const unsigned char *data, size_t size)
{
    CAST(bootImage);
    bi->setAbootImageC(data, size);
}

void mbp_bootimage_ipl_image(const CBootImage *bootImage,
                             const unsigned char **data, size_t *size)
{
    CCAST(bootImage);
    bi->iplImageC(data, size);
}

void mbp_bootimage_set_ipl_image(CBootImage *bootImage,
                                 const unsigned char *data, size_t size)
{
    CAST(bootImage);
    bi->setIplImageC(data, size);
}

void mbp_bootimage_rpm_image(const CBootImage *bootImage,
                             const unsigned char **data, size_t *size)
{
    CCAST(bootImage);
    bi->rpmImageC(data, size);
}

void mbp_bootimage_set_rpm_image(CBootImage *bootImage,
                                 const unsigned char *data, size_t size)
{
    CAST(bootImage);
    bi->setRpmImageC(data, size);
}

void mbp_bootimage_appsbl_image(const CBootImage *bootImage,
                                const unsigned char **data, size_t *size)
{
    CCAST(bootImage);
    bi->appsblImageC(data, size);
}

void mbp_bootimage_set_appsbl_image(CBootImage *bootImage,
                                    const unsigned char *data, size_t size)
{
    CAST(bootImage);
    bi->setAppsblImageC(data, size);
}

void mbp_bootimage_sin_image(const CBootImage *bootImage,
                             const unsigned char **data, size_t *size)
{
    CCAST(bootImage);
    bi->sinImageC(data, size);
}

void mbp_bootimage_set_sin_image(CBootImage *bootImage,
                                 const unsigned char *data, size_t size)
{
    CAST(bootImage);
    bi->setSinImageC(data, size);
}

void mbp_bootimage_sin_header(const CBootImage *bootImage,
                              const unsigned char **data, size_t *size)
{
    CCAST(bootImage);
    bi->sinHeaderC(data, size);
}

void mbp_bootimage_set_sin_header(CBootImage *bootImage,
                                  const unsigned char *data, size_t size)
{
    CAST(bootImage);
    bi->setSinHeaderC(data, size);
}

bool mbp_bootimage_equals(CBootImage *lhs, CBootImage *rhs)
{
    const mbp::BootImage *biLhs = reinterpret_cast<const mbp::BootImage *>(lhs);
    const mbp::BootImage *biRhs = reinterpret_cast<const mbp::BootImage *>(rhs);
    if (biLhs == biRhs) {
        return true;
    }
    if (!biLhs) {
        return false;
    }
    if (!biRhs) {
        return false;
    }
    return *biLhs == *biRhs;
}

}
