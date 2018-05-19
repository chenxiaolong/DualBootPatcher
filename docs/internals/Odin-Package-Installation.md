# Odin Package Installation

DualBootPatcher includes a special-purpose `update-binary` installer, named `odinupdater`, for TouchWiz Odin packages, which come with no installer because they're just a collection of images. `odinupdater` handles the flashing of the system image, the cache image (for the CSC/OMC carrier packages), and the boot image.


## `odinupdater`

`odinupdater` is an installer for patched Odin images. It makes certain assumptions about the installation environment and can only be used with DualBootPatcher. `odinupdater` expects the zip file to contain the following files:

* `system.img.sparse` - Android sparse image for system partition
* `cache.img.sparse` - Android sparse image for cache partition
* `boot.img` - Raw image for boot partition
* `fuse-sparse` - `fuse-sparse` binary
* `fuse-sparse.sig` - `fuse-sparse` signature

_NOTE: The digital signature for `fuse-sparse` is currently not checked. This will be implemented in the future._

### System Image

The system image is stored as an Android sparse image in the zip file. It is simply desparsed and extracted to the system block device (without any intermediate temporary files).

After flashing the system image, the RLC (remote lock control) mechanism will be removed to prevent the device from getting stuck in a 7 day bootloader lock period after the next reboot. DualBootPatcher normally does not make any functional modifications to ROMs, but this issue is significant enough to warrant it. The removal is done by:

* Deleting `/system/priv-app/Rlc`
* Changing the `ro.security.vaultkeeper.feature` property, if present, to `0` in `/system/build.prop` and `/vendor/build.prop`.

`com.samsung.android.rlc.service.RmmTask.registerRcv()` in `/system/framework/services.jar` checks that the RLC app is installed and that the property value is `1`. If either condition fails, RLC is disabled.

### CSC/OMC Package

The carrier package (CSC/OMC) is included in the cache partition sparse image within an Odin package. Inside the image, the carrier files are stored within a flashable ZIP file at `recovery/sec_csc.zip` or `recovery/sec_omc.zip`.

During the flashing process, the sparse image is extracted to `/tmp/cache.img.ext4` and then is mounted as a full (non-sparse) image at `/tmp/cache.img` via [`fuse-sparse`](#fuse_sparse). It's done this way for a couple reasons:

* Desparsing the image during installation would slow down the patching time since DEFLATE compression is slow, even when compressing a bunch of null bytes.
* Desparsing the image during extraction at flash time may cause an out of memory situation on some devices because the cache image size could be greater than the amount of memory. [`libmbsparse`](TODO) currently does not provide an API for querying for the holes in the sparse image. Thus, there's no way to store the raw image as a sparse file on the tmpfs.

Once the cache image is mounted, `system/*` from the carrier package ZIP file is extracted to the target installation.

Further actions may need to be performed depending the the carrier package type.

* Single CSC:
  * Nothing special needs to be done. `/system/csc` only contains files for one carrier.
* Multi CSC:
  * `/system/csc/common/system` is copied to `/system`
  * `/system/csc/<sales code>/system` is copied to `/system`. The sales code is read from `/efs/imei/mps_code.dat`. `/efs` is guaranteed to be mounted by [`mbtool_recovery`](TODO) on Samsung devices.
  * `/system/csc/<sales code>/csc_contents` is symlinked to `/system/csc_contents`
* OMC:
  * No special action needs to be taken. TouchWiz uses `/system/omc/sales_code.dat` for the sales code.

### Boot Image

Nothing special is done with the boot image. It is directly extracted from the zip file to the boot block device.


## `fuse-sparse`

`fuse-sparse` is a [FUSE](https://github.com/libfuse/libfuse)-based tool, built on top of [`libmbsparse`](TODO), for mounting an Android sparse image and exposing it as a raw image. It is useful when the raw image needs to be mounted, but converting the sparse image to the raw image is not feasible--for example, if the image needs to be stored in an archive or a filesystem that does not support sparse files.

`fuse-sparse` is compiled as a static executable, so the binaries shipped with DualBootPatcher can be used on any Linux system. To mount a sparse image and expose it as a raw image:

```sh
touch raw.img
fuse-sparse /path/to/sparse.img raw.img
```

To unmount the raw image:

```sh
fusermount -u raw.img
```

If the original sparse image file is seekable, then the raw image is seekable as well.

Note that `fuse-sparse` does not truly support concurrent reads from the same file descriptor to the raw image. Concurrent reads are safe, but are internally serialized.