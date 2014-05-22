package com.github.chenxiaolong.dualbootpatcher.switcher;

import java.io.File;
import java.io.FileInputStream;
import java.io.StringReader;
import java.util.ArrayList;
import java.util.Properties;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import android.content.Context;

import com.github.chenxiaolong.dualbootpatcher.CommandUtils.CommandResult;
import com.github.chenxiaolong.dualbootpatcher.CommandUtils.RootCommandListener;
import com.github.chenxiaolong.dualbootpatcher.CommandUtils.RootCommandParams;
import com.github.chenxiaolong.dualbootpatcher.CommandUtils.RootCommandRunner;
import com.github.chenxiaolong.dualbootpatcher.FileUtils;
import com.github.chenxiaolong.dualbootpatcher.R;

public class RomDetector {
    private static ArrayList<String> mRoms;

    public static String[] getRoms() {
        if (mRoms == null) {
            mRoms = new ArrayList<String>();

            // Primary - either:
            // - No /raw-system and have /system/build.prop
            // - Have /raw-system/build.prop
            if (!new File("/raw-system").exists()
                    && new File("/system/build.prop").exists()) {
                mRoms.add("/system");
            } else if (new File("/raw-system/build.prop").exists()) {
                mRoms.add("/raw-system");
            }

            if (FileUtils.isExistsDirectory("/raw-system/dual")) {
                mRoms.add("/raw-system/dual");
            } else if (FileUtils.isExistsDirectory("/system/dual")) {
                mRoms.add("/system/dual");
            }

            int max = 10;
            for (int i = 0; i < max; i++) {
                if (FileUtils.isExistsDirectory("/raw-cache/multi-slot-" + i + "/system")) {
                    mRoms.add("/raw-cache/multi-slot-" + i + "/system");
                } else if (FileUtils.isExistsDirectory("/cache/multi-slot-" + i + "/system")) {
                    mRoms.add("/cache/multi-slot-" + i + "/system");
                }
            }
        }

        return mRoms.toArray(new String[mRoms.size()]);
    }

    private static String getDefaultName(Context context, String rom) {
        if (rom.equals("/system") || rom.equals("/raw-system")) {
            return context.getString(R.string.primary);
        } else if (rom.contains("system/dual")) {
            return context.getString(R.string.secondary);
        } else if (rom.contains("cache/multi-slot-")) {
            Pattern p = Pattern
                    .compile("^/(?:raw-)?cache/multi-slot-([^/]+)/system$");
            Matcher m = p.matcher(rom);
            String num = "UNKNOWN";
            if (m.find()) {
                num = m.group(1);
            }
            return String.format(context.getString(R.string.multislot), num);
        } else {
            return "UNKNOWN";
        }
    }

    public static String getName(Context context, String rom) {
        // TODO: Allow renaming
        return getDefaultName(context, rom);
    }

    public static String getId(String rom) {
        if (rom.equals("/system") || rom.equals("/raw-system")) {
            return "primary";
        } else if (rom.contains("system/dual")) {
            return "secondary";
        } else if (rom.contains("cache/multi-slot-")) {
            Pattern p = Pattern
                    .compile("^/(?:raw-)?cache/(multi-slot-[^/]+)/system$");
            Matcher m = p.matcher(rom);
            if (m.find()) {
                return m.group(1);
            } else {
                return null;
            }
        } else {
            return null;
        }
    }

    private static Properties getBuildProp(String rom) {
        File buildprop = new File(rom + "/build.prop");

        // Make sure build.prop exists
        if (!buildprop.exists()) {
            try {
                RootCommandParams params = new RootCommandParams();
                params.command = "ls " + buildprop.getPath();

                RootCommandRunner cmd = new RootCommandRunner(params);
                cmd.start();
                cmd.join();
                CommandResult result = cmd.getResult();

                if (result.exitCode != 0) {
                    return null;
                }
            } catch (Exception e) {
                return null;
            }
        }

        Properties prop = new Properties();

        if (!buildprop.canRead()) {
            try {
                RootCommandParams params = new RootCommandParams();
                params.command = "cat " + buildprop.getPath();
                params.listener = new FullOutputListener();

                RootCommandRunner cmd = new RootCommandRunner(params);
                cmd.start();
                cmd.join();
                CommandResult result = cmd.getResult();

                if (result.exitCode != 0) {
                    return null;
                }

                String output = result.data.getString("output");

                if (output == null) {
                    return null;
                }

                prop.load(new StringReader(output));
            } catch (Exception e) {
                return null;
            }
        } else {
            FileInputStream fis = null;
            try {
                fis = new FileInputStream(buildprop.getPath());
                prop.load(fis);
            } catch (Exception e) {
                return null;
            } finally {
                try {
                    if (fis != null) {
                        fis.close();
                    }
                } catch (Exception e) {
                }
            }
        }

        return prop;
    }

    public static String getVersion(String rom) {
        Properties prop = getBuildProp(rom);

        if (prop == null) {
            return null;
        }

        if (prop.containsKey("ro.modversion")) {
            return prop.getProperty("ro.modversion");
        } else if (prop.containsKey("ro.slim.version")) {
            return prop.getProperty("ro.slim.version");
        } else if (prop.containsKey("ro.cm.version")) {
            return prop.getProperty("ro.cm.version");
        } else if (prop.containsKey("ro.omni.version")) {
            return prop.getProperty("ro.omni.version");
        } else if (prop.containsKey("ro.build.display.id")) {
            return prop.getProperty("ro.build.display.id");
        } else {
            return null;
        }
    }

    public static int getIconResource(String rom) {
        Properties prop = getBuildProp(rom);

        if (prop == null) {
            return R.drawable.rom_android;
        }

        if (prop.containsKey("ro.slim.version")) {
            return R.drawable.rom_slimroms;
        } else if (prop.containsKey("ro.cm.version")) {
            return R.drawable.rom_cyanogenmod;
        } else if (prop.containsKey("ro.omni.version")) {
            return R.drawable.rom_omnirom;
        } else {
            return R.drawable.rom_android;
        }
    }

    private static class FullOutputListener implements RootCommandListener {
        private final StringBuilder mOutput = new StringBuilder();

        @Override
        public void onNewOutputLine(String line) {
            mOutput.append(line);
            mOutput.append(System.getProperty("line.separator"));
        }

        @Override
        public void onCommandCompletion(CommandResult result) {
            result.data.putString("output", mOutput.toString());
        }
    }
}
