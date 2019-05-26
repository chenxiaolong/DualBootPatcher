Due to recent changes in Android P, as well as upcoming changes in Android Q, DualBootPatcher is no longer being developed. These two releases change some fundamental assumptions that DBP makes about the host device.

With devices that ship with Android 9.0+, the [system-as-root partition layout](https://source.android.com/devices/bootloader/system-as-root) is mandatory. This means that the system partition includes everything that traditionally went in the boot image's ramdisk. DBP relies on being able to modify the ramdisk to add its binaries as well as some config files that specify the ROM ID and device specific information (eg. partitions). To switch between ROMs, DBP simply flashes the ROM's patched boot image.

With the system-as-root partition layout, most of the files could potentially live on the system partition, but the ROM ID must be stored in the boot image. With some devices, like Google Pixels, a ramdisk can be added back by including it in the boot image and patching the kernel image to ignore the `skip_initramfs` cmdline option. However, on other devices, like the Samsung Galaxy S10 series, the bootloader will always ignore the ramdisk section in the boot image. Storing the ROM ID in the cmdline field is also not feasible because many devices' bootloaders ignore the whole field.

With the Android Q preview builds, some devices, such as the Google Pixel 3 series, switched to using [dm-linear](https://www.linuxplumbersconf.org/event/2/contributions/225/attachments/49/56/06._Dynamic_Partitions_-_LPC_Android_MC_v2.pdf) for handling the partition layout of the read-only OS partitions (system, vendor, etc.). With this setup, a single GPT partition is split up and is mapped to device-mapper block devices by a userspace tool. With Android's `/init`, this is done via the `liblp` library, which parses the metadata on disk and configures dm-linear. DBP would have to implement something equivalent to this to be able to mount the Android partitions. It currently assumes that the kernel will provide a mountable block device after going throug the uevent device probing phase.

Neither of these changes are impossible to work around, but I have simply lost any interest in doing so. I have not used DBP on my primary devices for a couple of years now. Those interested in continuing development are free to fork the project. Any work that had been done for the 10.0.0 release has been pushed to the `10.0.0-staging` branch.

All downloads have been moved to SourceForge at https://sourceforge.net/projects/dualbootpatcher/files/. I will work on splitting out some of the more useful parts of DBP, such as libmbbootimg (boot image parser) and libmbsystrace (syscall injection/modification library), so that they can be used in other projects, but DBP itself will no longer be developed.

Huge thanks to everyone who helped out with this project the past six years!

---

Downloads
---------

All previous builds can be found [here](https://sourceforge.net/projects/dualbootpatcher/files/).


Compiling from Source
---------------------

See the [`docs/build/`](docs/build) directory for instructions on building for Linux, Windows, and Android.


License
-------

The patcher is licensed under GPLv3+ (see the LICENSE file). Third party libraries and programs are used under their respective licenses. Copies of these licenses are in the `licenses/` directory of this repository. Patches and other source code modifications to third party software are under the same license as the original software.
