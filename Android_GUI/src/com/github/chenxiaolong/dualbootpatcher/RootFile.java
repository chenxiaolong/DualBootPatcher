/*
 * Copyright (C) 2014  Xiao-Long Chen <chenxiaolong@cxl.epac.to>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

package com.github.chenxiaolong.dualbootpatcher;

import android.util.Log;

import com.github.chenxiaolong.dualbootpatcher.CommandUtils.CommandParams;
import com.github.chenxiaolong.dualbootpatcher.CommandUtils.CommandResult;
import com.github.chenxiaolong.dualbootpatcher.CommandUtils.CommandRunner;
import com.github.chenxiaolong.dualbootpatcher.CommandUtils.FullRootOutputListener;
import com.github.chenxiaolong.dualbootpatcher.CommandUtils.RootCommandListener;
import com.github.chenxiaolong.dualbootpatcher.CommandUtils.RootCommandParams;
import com.github.chenxiaolong.dualbootpatcher.CommandUtils.RootCommandRunner;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.util.ArrayList;

public class RootFile {
    public static final String TAG = RootFile.class.getSimpleName();

    public boolean mAttemptRoot = true;
    public File mFile;

    public RootFile(String path) {
        mFile = new File(path);
    }

    public RootFile(String path, boolean attemptRoot) {
        mFile = new File(path);
        mAttemptRoot = attemptRoot;
    }

    public boolean getAttemptRoot() {
        return mAttemptRoot;
    }

    public void setAttemptRoot(boolean attemptRoot) {
        mAttemptRoot = attemptRoot;
    }

    public boolean isDirectory() {
        if (mFile.exists() && mFile.isDirectory() && mFile.canRead()) {
            return true;
        } else if (mAttemptRoot) {
            return CommandUtils.runRootCommand("test -d " + getAbsolutePath()) == 0;
        }

        return false;
    }

    public boolean isFile() {
        if (mFile.exists() && mFile.isFile() && mFile.canRead()) {
            return true;
        } else if (mAttemptRoot) {
            return CommandUtils.runRootCommand("test -f " + getAbsolutePath()) == 0;
        }

        return false;
    }

    public String getAbsolutePath() {
        String path = mFile.getAbsolutePath();

        if (path != null) {
            return path;
        } else if (mAttemptRoot) {
            RootCommandParams params = new RootCommandParams();
            params.command = "readlink -f " + mFile;
            params.listener = new FirstLineRootOutputListener();

            RootCommandRunner cmd = new RootCommandRunner(params);
            cmd.start();
            CommandUtils.waitForRootCommand(cmd);

            CommandResult result = cmd.getResult();

            if (result.exitCode != 0) {
                return null;
            }

            String line = result.data.getString(FirstLineRootOutputListener.FIRST_LINE);

            if (line == null) {
                return null;
            }

            return line;
        }

        return null;
    }

    public void delete() {
        mFile.delete();

        if (mAttemptRoot) {
            if (isFile()) {
                CommandUtils.runRootCommand("rm -f " + getAbsolutePath());
            } else if (isDirectory()) {
                CommandUtils.runRootCommand("rmdir " + getAbsolutePath());
            }
        }
    }

    public void recursiveDelete() {
        if (isFile()) {
            delete();
        } else if (isDirectory()) {
            String[] files = list();

            if (files != null) {
                for (String file : files) {
                    new RootFile(file, mAttemptRoot).recursiveDelete();
                }
            }

            delete();
        }
    }

    public void moveTo(File dest) {
        moveTo(new RootFile(dest.toString()));
    }

    public void moveTo(RootFile dest) {
        try {
            org.apache.commons.io.FileUtils.moveFile(mFile, dest.mFile);
        } catch (Exception e) {
            e.printStackTrace();

            if (mAttemptRoot) {
                CommandUtils.runRootCommand("cp " + getAbsolutePath()
                        + " " + dest.getAbsolutePath());
                delete();
            }
        }
    }

    public String[] list() {
        if (!isDirectory()) {
            return null;
        }

        if (mFile.exists() && mFile.isDirectory() && mFile.canRead()) {
            return mFile.list();
        }

        if (!mAttemptRoot) {
            return null;
        }

        try {
            RootCommandParams params = new RootCommandParams();
            params.command = "ls " + getAbsolutePath();
            params.listener = new FullRootOutputListener();

            RootCommandRunner cmd = new RootCommandRunner(params);
            cmd.start();
            CommandUtils.waitForRootCommand(cmd);

            CommandResult result = cmd.getResult();

            if (result.exitCode != 0) {
                return null;
            }

            String output = result.data.getString("output");

            if (output == null) {
                return null;
            }

            String newline = System.getProperty("line.separator");
            String[] split = output.split(newline);
            ArrayList<String> files = new ArrayList<String>();

            for (String s : split) {
                if (!s.isEmpty()) {
                    files.add(s);
                }
            }

            return files.toArray(new String[files.size()]);
        } catch (Exception e) {
            return null;
        }
    }

    public boolean mkdirs() {
        if (isDirectory()) {
            return true;
        } else if (mFile.mkdirs()) {
            return true;
        } else if (mAttemptRoot) {
            CommandUtils.runRootCommand("mkdir -p " + getAbsolutePath());
            return isDirectory();
        }

        return false;
    }

    public void chown(int uid, int gid) {
        chown(Integer.toString(uid), Integer.toString(gid));
    }

    public void chown(String uid, String gid) {
        CommandParams params = new CommandParams();
        params.command = new String[] { "chown", uid + ":" + gid, getAbsolutePath() };

        CommandRunner cmd = new CommandRunner(params);
        cmd.start();
        CommandUtils.waitForCommand(cmd);

        CommandResult result = cmd.getResult();

        if (mAttemptRoot && (result == null || result.exitCode != 0)) {
            CommandUtils.runRootCommand("chown " + uid + ":" + gid + " " + getAbsolutePath());
        }
    }

    public void recursiveChown(int uid, int gid) {
        recursiveChown(Integer.toString(uid), Integer.toString(gid));
    }

    public void recursiveChown(String uid, String gid) {
        CommandParams params = new CommandParams();
        params.command = new String[] { "chown", "-R", uid + ":" + gid, getAbsolutePath() };

        CommandRunner cmd = new CommandRunner(params);
        cmd.start();
        CommandUtils.waitForCommand(cmd);

        CommandResult result = cmd.getResult();

        if (mAttemptRoot && (result == null || result.exitCode != 0)) {
            CommandUtils.runRootCommand("chown -R " + uid + ":" + gid + " " + getAbsolutePath());
        }
    }

    public void chmod(int octal) {
        CommandParams params = new CommandParams();
        params.command = new String[] { "chmod", Integer.toOctalString(octal), getAbsolutePath() };

        CommandRunner cmd = new CommandRunner(params);
        cmd.start();
        CommandUtils.waitForCommand(cmd);

        CommandResult result = cmd.getResult();

        if (mAttemptRoot && (result == null || result.exitCode != 0)) {
            CommandUtils.runRootCommand("chmod "
                    + Integer.toOctalString(octal) + " " + getAbsolutePath());
        }
    }

    public void recursiveChmod(int octal) {
        CommandParams params = new CommandParams();
        params.command = new String[] { "chmod", "-R", Integer.toOctalString(octal),
                mFile.getAbsolutePath() };

        CommandRunner cmd = new CommandRunner(params);
        cmd.start();
        CommandUtils.waitForCommand(cmd);

        CommandResult result = cmd.getResult();

        if (mAttemptRoot && (result == null || result.exitCode != 0)) {
            CommandUtils.runRootCommand("chmod -R "
                    + Integer.toOctalString(octal) + " " + mFile.getAbsolutePath());
        }
    }

    public String getContents() {
        if (!isFile()) {
            return null;
        }

        if (mFile.canRead()) {
            BufferedReader br = null;
            try {
                br = new BufferedReader(new FileReader(getAbsolutePath()));
                StringBuilder sb = new StringBuilder();
                String line;

                while ((line = br.readLine()) != null) {
                    sb.append(line);
                    sb.append(System.getProperty("line.separator"));
                    //sb.append(System.lineSeparator());
                }
                return sb.toString();
            } catch (IOException e) {
                e.printStackTrace();
            } finally {
                if (br != null) {
                    try {
                        br.close();
                    } catch (IOException e) {
                        e.printStackTrace();
                    }
                }
            }
        } else if (mAttemptRoot) {
            RootCommandParams params = new RootCommandParams();
            params.command = "cat " + getAbsolutePath();
            params.listener = new FullRootOutputListener();

            RootCommandRunner cmd = new RootCommandRunner(params);
            cmd.start();
            CommandUtils.waitForRootCommand(cmd);

            CommandResult result = cmd.getResult();

            if (result.exitCode != 0) {
                return null;
            }

            String output = result.data.getString("output");

            if (output == null) {
                return null;
            }

            return output;
        }

        return null;
    }

    public void writeContents(String contents) {
        boolean success = false;

        try {
            if (!mFile.exists()) {
                success = mFile.createNewFile();
            } else if (mFile.canWrite()) {
                success = true;
            }
        } catch (IOException e) {
            e.printStackTrace();
        }

        if (success) {
            BufferedWriter bw = null;
            try {
                bw = new BufferedWriter(new FileWriter(getAbsolutePath()));
                bw.write(contents);
            } catch (IOException e) {
                e.printStackTrace();
            } finally {
                if (bw != null) {
                    try {
                        bw.close();
                    } catch (IOException e) {
                        e.printStackTrace();
                    }
                }
            }
        } else if (mAttemptRoot) {
            CommandUtils.runRootCommand("echo '" + contents + "' > " + getAbsolutePath());
        }
    }

    public boolean isTmpFsMounted() {
        BufferedReader br = null;
        try {
            br = new BufferedReader(new FileReader("/proc/mounts"));
            String line;

            while ((line = br.readLine()) != null) {
                String[] split = line.split(" ");
                // See man:fstab(8)
                String fsSpec = split[0];
                String fsFile = split[1];
                String fsVfsType = split[2];
                String fsMntOps = split[3];
                String fsFreq = split[4];
                String fsPassNo = split[5];

                // We can ignore escaped characters because the tmpfs paths used are alphanumeric
                if (fsVfsType.equals("tmpfs") && fsFile.equals(mFile.getPath())) {
                    return true;
                }
            }
        } catch (IOException e) {
            e.printStackTrace();
        } finally {
            if (br != null) {
                try {
                    br.close();
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
        }

        return false;
    }

    public boolean mountTmpFs() {
        if (!mAttemptRoot) {
            return false;
        }

        if (isTmpFsMounted()) {
            Log.w(TAG, "A tmpfs is already mounted. Skipping tmpfs mounting");
            return true;
        }

        if (isDirectory()) {
            recursiveDelete();
        }

        mkdirs();
        return CommandUtils.runRootCommand("mount -t tmpfs tmpfs " + getAbsolutePath()) == 0;
    }

    public void unmountTmpFs() {
        if (!mAttemptRoot) {
            return;
        }

        if (!isTmpFsMounted()) {
            return;
        }

        if (isDirectory()) {
            CommandUtils.runRootCommand("umount " + getAbsolutePath());
        }
    }

    public static class FirstLineRootOutputListener implements RootCommandListener {
        private static final String FIRST_LINE = "first_line";

        private String mLine;

        @Override
        public void onNewOutputLine(String line) {
            if (mLine == null) {
                mLine = line;
            }
        }

        @Override
        public void onCommandCompletion(CommandResult result) {
            result.data.putString(FIRST_LINE, mLine);
        }
    }
}
