# Building DualBootUtilities Zip

Please follow the directions in [`BUILD-App-Android.md`](BUILD-App-Android.md) up until the build step. The same dependencies are needed for building the utilities zip.

## Build process

After running `cmake`:

1. Build the `armeabi-v7a` components. Building everything with just `make` will work as well, but this will save time during the compilation.

   ```sh
   make android-system_armeabi-v7a
   ```

2. Build `devices.json` from the templates in `data/devices`.

   ```sh
   make -C data/devices
   ```

3. Build the utilities zip.

   ```sh
   ./utilities/create.sh
   ```

   The result will be in `utilities/DualBootUtilities-<version>.zip`
