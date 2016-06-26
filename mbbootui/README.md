Description
-----------

mbbootui is based on the graphical code from TWRP. Nearly all of the TWRP-specific functionality has been stripped from the code. The following lists the major changes made to the code:

* The entire graphics system is configured at runtime instead of at compile time using ifdefs. As such, a single binary can run on any device, provided that it is properly configured.
* Removed all legacy themes and new scalable themes, except for `portrait_hdpi`.
* Removed duplicate functions that can be provided by other DualBootPatcher libraries, such as `libmbutil`.
* Made the code style consistent.

The original TWRP sources can be found here:
    https://github.com/omnirom/android_bootable_recovery
