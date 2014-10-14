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

#include "patchererror.h"

PatcherError::PatcherError()
{
}

QString PatcherError::errorString(Error error)
{
    switch (error) {
    case NoError:
        return tr("No error has occurred");

    case CustomError:
        // String should be set by the code that produces the error
        return QString();

    case UnknownError:
        return tr("An unknown error has occurred");

    case ImplementationError:
        return tr("The dualbootpatcher library wasn't called properly");

    case PatcherCreateError:
        return tr("Failed to create patcher: %1");

    case AutoPatcherCreateError:
        return tr("Failed to create autopatcher: %1");

    case RamdiskPatcherCreateError:
        return tr("Failed to create ramdisk patcher: %1");

    case FileOpenError:
        return tr("Failed to open file: %1");

    case FileMapError:
        return tr("Failed to map file to memory: %1");

    case FileWriteError:
        return tr("Failed to write to file: %1");

    case DirectoryNotExistError:
        return tr("Directory does not exist: %1");

    case BootImageSmallerThanHeaderError:
        return tr("The boot image file is smaller than the boot image header size");

    case BootImageNoAndroidHeaderError:
        return tr("Could not find the Android header in the boot image");

    case BootImageNoRamdiskGzipHeaderError:
        return tr("Could not find the ramdisk's gzip header");

    case BootImageNoRamdiskSizeError:
        return tr("Could not determine the ramdisk's size");

    case BootImageNoKernelSizeError:
        return tr("Could not determine the kernel's size");

    case BootImageNoRamdiskAddressError:
        return tr("Could not determine the ramdisk's memory address");

    case CpioFileAlreadyExistsError:
        return tr("File already exists in cpio archive: %1");

    case CpioFileNotExistError:
        return tr("File does not exist in cpio archive: %1");

    case LibArchiveReadOpenError:
        return tr("Failed to open archive for reading");

    case LibArchiveReadDataError:
        return tr("Failed to read archive data for file: %1");

    case LibArchiveReadHeaderError:
        return tr("Failed to read archive entry header");

    case LibArchiveWriteOpenError:
        return tr("Failed to open archive for writing");

    case LibArchiveWriteDataError:
        return tr("Failed to write archive data for file: %1");

    case LibArchiveWriteHeaderError:
        return tr("Failed to write archive header for file: %1");

    case LibArchiveCloseError:
        return tr("Failed to close archive");

    case LibArchiveFreeError:
        return tr("Failed to free archive header memory");

    case XmlParseFileError:
        return tr("Failed to parse XML file: %1");

    case OnlyZipSupported:
        return tr("Only ZIP files are supported by %1");

    case OnlyBootImageSupported:
        return tr("Only boot images are supported by %1");

    default:
        return QString();
    }
}
