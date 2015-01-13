/*
 * Copyright (C) 2011 asksven
 * Copyright (C) 2014 Xiao-Long Chen <chenxiaolong@cxl.epac.to>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.github.chenxiaolong.dualbootpatcher;


import java.io.File;
import java.lang.reflect.Method;

import android.content.Context;

import dalvik.system.DexFile;


public class SystemPropertiesProxy {
    private static final String SYSTEM_PROPERTIES = "android.os.SystemProperties";

    /**
     * This class cannot be instantiated
     */
    private SystemPropertiesProxy() {
    }

    /**
     * Get the value for the given key.
     * @return an empty string if the key isn't found
     * @throws IllegalArgumentException if the key exceeds 32 characters
     */
    public static String get(Context context, String key) throws IllegalArgumentException {
        String ret;

        try {
            ClassLoader cl = context.getClassLoader();
            @SuppressWarnings("rawtypes")
            Class SystemProperties = cl.loadClass(SYSTEM_PROPERTIES);

            @SuppressWarnings("unchecked")
            Method get = SystemProperties.getMethod("get", String.class);
            ret = (String) get.invoke(SystemProperties, key);
        } catch (IllegalArgumentException e) {
            throw e;
        } catch (Exception e) {
            ret = "";
        }

        return ret;
    }

    /**
     * Get the value for the given key.
     * @return if the key isn't found, return def if it isn't null, or an empty string otherwise
     * @throws IllegalArgumentException if the key exceeds 32 characters
     */
    public static String get(Context context, String key,
                             String def) throws IllegalArgumentException {

        String ret;

        try {
            ClassLoader cl = context.getClassLoader();
            @SuppressWarnings("rawtypes")
            Class SystemProperties = cl.loadClass(SYSTEM_PROPERTIES);

            @SuppressWarnings("unchecked")
            Method get = SystemProperties.getMethod("get", String.class, String.class);
            ret = (String) get.invoke(SystemProperties, key, def);
        } catch (IllegalArgumentException e) {
            throw e;
        } catch (Exception e) {
            ret = def;
        }

        return ret;
    }

    /**
     * Get the value for the given key, and return as an integer.
     * @param key the key to lookup
     * @param def a default value to return
     * @return the key parsed as an integer, or def if the key isn't found or
     * cannot be parsed
     * @throws IllegalArgumentException if the key exceeds 32 characters
     */
    public static Integer getInt(Context context, String key,
                                 int def) throws IllegalArgumentException {
        Integer ret;

        try {
            ClassLoader cl = context.getClassLoader();
            @SuppressWarnings("rawtypes")
            Class SystemProperties = cl.loadClass(SYSTEM_PROPERTIES);

            @SuppressWarnings("unchecked")
            Method getInt = SystemProperties.getMethod("getInt", String.class, int.class);
            ret = (Integer) getInt.invoke(SystemProperties, key, def);
        } catch (IllegalArgumentException e) {
            throw e;
        } catch (Exception e) {
            ret = def;
        }

        return ret;
    }

    /**
     * Get the value for the given key, and return as a long.
     * @param key the key to lookup
     * @param def a default value to return
     * @return the key parsed as a long, or def if the key isn't found or
     * cannot be parsed
     * @throws IllegalArgumentException if the key exceeds 32 characters
     */
    public static Long getLong(Context context, String key,
                               long def) throws IllegalArgumentException {

        Long ret;

        try {
            ClassLoader cl = context.getClassLoader();
            @SuppressWarnings("rawtypes")
            Class SystemProperties = cl.loadClass(SYSTEM_PROPERTIES);

            @SuppressWarnings("unchecked")
            Method getLong = SystemProperties.getMethod("getLong", String.class, long.class);
            ret = (Long) getLong.invoke(SystemProperties, key, def);
        } catch (IllegalArgumentException e) {
            throw e;
        } catch (Exception e) {
            ret = def;
        }

        return ret;
    }

    /**
     * Get the value for the given key, returned as a boolean.
     * Values 'n', 'no', '0', 'false' or 'off' are considered false.
     * Values 'y', 'yes', '1', 'true' or 'on' are considered true.
     * (case insensitive).
     * If the key does not exist, or has any other value, then the default
     * result is returned.
     * @param key the key to lookup
     * @param def a default value to return
     * @return the key parsed as a boolean, or def if the key isn't found or is
     * not able to be parsed as a boolean.
     * @throws IllegalArgumentException if the key exceeds 32 characters
     */
    public static Boolean getBoolean(Context context, String key,
                                     boolean def) throws IllegalArgumentException {

        Boolean ret;

        try {
            ClassLoader cl = context.getClassLoader();
            @SuppressWarnings("rawtypes")
            Class SystemProperties = cl.loadClass(SYSTEM_PROPERTIES);

            @SuppressWarnings("unchecked")
            Method getBoolean = SystemProperties.getMethod(
                    "getBoolean", String.class, boolean.class);

            ret = (Boolean) getBoolean.invoke(SystemProperties, key, def);
        } catch (IllegalArgumentException e) {
            throw e;
        } catch (Exception e) {
            ret = def;
        }

        return ret;
    }

    /**
     * Set the value for the given key.
     * @throws IllegalArgumentException if the key exceeds 32 characters
     * @throws IllegalArgumentException if the value exceeds 92 characters
     */
    public static void set(Context context, String key,
                           String val) throws IllegalArgumentException {
        try {
            @SuppressWarnings("unused")
            DexFile df = new DexFile(new File("/system/app/Settings.apk"));
            @SuppressWarnings("unused")
            ClassLoader cl = context.getClassLoader();
            @SuppressWarnings("rawtypes")
            Class SystemProperties = Class.forName(SYSTEM_PROPERTIES);

            @SuppressWarnings("unchecked")
            Method set = SystemProperties.getMethod("set", String.class, String.class);
            set.invoke(SystemProperties, key, val);

        } catch (IllegalArgumentException e) {
            throw e;
        } catch (Exception e) {
        }
    }
}