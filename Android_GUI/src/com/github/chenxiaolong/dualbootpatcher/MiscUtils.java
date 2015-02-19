/*
 * Copyright (C) 2014  Andrew Gunnerson <andrewgunnerson@gmail.com>
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
import java.util.Properties;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class MiscUtils {
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

            // No suffix is older (suffix is n revisions ahead of the last git tag)
            if (mSuffix != null && other.mSuffix == null) {
                return 1;
            } else if (mSuffix == null && other.mSuffix != null) {
                return -1;
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
}
