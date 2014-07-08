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

import java.io.IOException;
import java.io.StringReader;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Properties;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class MiscUtils {
    public static final int FEATURE_GLOBAL_APP_SHARING = 1;
    public static final int FEATURE_GLOBAL_PAID_APP_SHARING = 2;
    public static final int FEATURE_INDIV_APP_SYNCING = 4;

    public static class Version implements Comparable<Version> {
        private int mMajorVer;
        private int mMinorVer;
        private int mVeryMinorVer;
        public String mSuffix;
        private int mRevision;
        private String mGitCommit;

        public Version(String versionString) {
            parseVersion(versionString);
        }

        private void parseVersion(String versionString) {
            Pattern p = Pattern.compile("(\\d+)\\.(\\d+)\\.(\\d+)(.*?)?(?:-.+)?");
            Matcher m = p.matcher(versionString);

            if (!m.matches()) {
                throw new IllegalArgumentException("Invalid version number");
            }

            mMajorVer = Integer.parseInt(m.group(1));
            mMinorVer = Integer.parseInt(m.group(2));
            mVeryMinorVer = Integer.parseInt(m.group(3));
            String suffix = m.group(4);
            if (suffix != null && !suffix.isEmpty()) {
                mSuffix = m.group(4);
            }

            if (mSuffix != null) {
                parseRevision();
            }
        }

        private void parseRevision() {
            Pattern p = Pattern.compile("\\.r(\\d+)(?:\\.g(.+))?");
            Matcher m = p.matcher(mSuffix);

            if (!m.matches()) {
                throw new IllegalArgumentException("Invalid version suffix");
            }

            mRevision = Integer.parseInt(m.group(1));
            mGitCommit = m.group(2);
        }

        @Override
        public int compareTo(Version other) {
            if (mMajorVer < other.mMajorVer) {
                return -1;
            } else if (mMajorVer > other.mMajorVer) {
                return 1;
            }

            if (mMinorVer < other.mMinorVer) {
                return -1;
            } else if (mMinorVer > other.mMinorVer) {
                return 1;
            }

            if (mVeryMinorVer < other.mVeryMinorVer) {
                return -1;
            } else if (mVeryMinorVer > other.mVeryMinorVer) {
                return 1;
            }

            // No suffix is always newer
            if (mSuffix != null && other.mSuffix == null) {
                return -1;
            } else if (mSuffix == null && other.mSuffix != null) {
                return 1;
            }

            if (mRevision < other.mRevision) {
                return -1;
            } else if (mRevision > other.mRevision) {
                return 1;
            }

            return 0;
        }

        @Override
        public boolean equals(Object other) {
            if (this == other) {
                return true;
            }

            if (!(other instanceof Version)) {
                return false;
            }

            Version vOther = (Version) other;

            return mMajorVer == vOther.mMajorVer
                    && mMinorVer == vOther.mMinorVer
                    && mVeryMinorVer == vOther.mVeryMinorVer
                    && mSuffix == null ? vOther.mSuffix == null :
                            mSuffix.equals(vOther.mSuffix)
                    && mRevision == vOther.mRevision
                    && mGitCommit == null ? vOther.mGitCommit == null :
                            mGitCommit.equals(vOther.mGitCommit);
        }

        @Override
        public String toString() {
            StringBuilder sb = new StringBuilder();
            sb.append(mMajorVer).append(".");
            sb.append(mMinorVer).append(".");
            sb.append(mVeryMinorVer);
            if (mRevision > 0) {
                sb.append(".r").append(mRevision);
            }
            if (mGitCommit != null) {
                sb.append(".g").append(mGitCommit);
            }
            return sb.toString();
        }

        public String dump() {
            return "mMajorVer: " + mMajorVer
                    + ", mMinorVer: " + mMinorVer
                    + ", mVeryMinorVer: " + mVeryMinorVer
                    + ", mSuffix: " + mSuffix
                    + ", mRevision: " + mRevision
                    + ", mGitCommit: " + mGitCommit;
        }
    }

    public static Version getMinimumVersionFor(int features) {
        ArrayList<Version> versions = new ArrayList<Version>();

        // To find the minimum version for continuous integration builds, run the following command:
        //   $ git describe --long | sed -E "s/^v//g;s/([^-]*-g)/r\1/;s/-/./g"
        // If the command returns "7.0.0.r56.gdd3907d", then then minimum version (uncommitted
        // changes) should be "7.0.0.r57".
        //
        // Release and debug versions should simply use the release version numbers (eg. X.Y.Z)
        if (BuildConfig.BUILD_TYPE.equals("ci")) {
            if ((features & FEATURE_GLOBAL_APP_SHARING) != 0) {
                Version v = new Version("7.0.0.r57");
                versions.add(v);
            }

            if ((features & FEATURE_GLOBAL_PAID_APP_SHARING) != 0) {
                Version v = new Version("7.0.0.r57");
                versions.add(v);
            }

            if ((features & FEATURE_INDIV_APP_SYNCING) != 0) {
                Version v = new Version("7.0.0.r57");
                versions.add(v);
            }
        } else {
            if ((features & FEATURE_GLOBAL_APP_SHARING) != 0) {
                Version v = new Version("8.0.0");
                versions.add(v);
            }

            if ((features & FEATURE_GLOBAL_PAID_APP_SHARING) != 0) {
                Version v = new Version("8.0.0");
                versions.add(v);
            }

            if ((features & FEATURE_INDIV_APP_SYNCING) != 0) {
                Version v = new Version("8.0.0");
                versions.add(v);
            }
        }

        if (versions.size() == 0) {
            return null;
        }

        // Find maximum (which is the minimum version needed to support all of the requested
        // features)
        return Collections.max(versions);
    }

    public static String getPatchedByVersion() {
        Properties prop = getDefaultProp();

        if (prop == null) {
            return "0.0.0";
        }

        if (prop.containsKey("ro.patcher.version")) {
            return prop.getProperty("ro.patcher.version");
        } else {
            return "0.0.0";
        }
    }

    public static String getPartitionConfig() {
        Properties prop = getDefaultProp();

        if (prop == null) {
            return null;
        }

        if (prop.containsKey("ro.patcher.patched")) {
            return prop.getProperty("ro.patcher.patched");
        } else {
            return null;
        }
    }

    public static Properties getProperties(String file) {
        // Make sure build.prop exists
        if (!new RootFile(file).isFile()) {
            return null;
        }

        Properties prop = new Properties();

        String contents = new RootFile(file).getContents();

        if (contents == null) {
            return null;
        }

        try {
            prop.load(new StringReader(contents));
            return prop;
        } catch (IOException e) {
            e.printStackTrace();
        }

        return null;
    }

    public static Properties getDefaultProp() {
        return getProperties("/default.prop");
    }
}
