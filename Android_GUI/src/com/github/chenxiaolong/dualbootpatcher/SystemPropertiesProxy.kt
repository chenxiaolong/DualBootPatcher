/*
 * Copyright (C) 2011 asksven
 * Copyright (C) 2014 Andrew Gunnerson <andrewgunnerson@gmail.com>
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

package com.github.chenxiaolong.dualbootpatcher

import android.annotation.SuppressLint
import android.content.Context

@SuppressLint("PrivateApi")
object SystemPropertiesProxy {
    private const val SYSTEM_PROPERTIES = "android.os.SystemProperties"

    /**
     * Get the value for the given key.
     * @return an empty string if the key isn't found
     * @throws IllegalArgumentException if the key exceeds 32 characters
     */
    @Throws(IllegalArgumentException::class)
    operator fun get(context: Context, key: String): String {
        return try {
            val cl = context.classLoader
            val sp = cl.loadClass(SYSTEM_PROPERTIES)

            val get = sp.getMethod("get", String::class.java)
            get.invoke(sp, key) as String
        } catch (e: IllegalArgumentException) {
            throw e
        } catch (e: Exception) {
            ""
        }
    }

    /**
     * Get the value for the given key.
     * @return if the key isn't found, return def if it isn't null, or an empty string otherwise
     * @throws IllegalArgumentException if the key exceeds 32 characters
     */
    @Throws(IllegalArgumentException::class)
    operator fun get(context: Context, key: String, def: String): String {
        return try {
            val cl = context.classLoader
            val sp = cl.loadClass(SYSTEM_PROPERTIES)

            val get = sp.getMethod("get", String::class.java, String::class.java)
            get.invoke(sp, key, def) as String
        } catch (e: IllegalArgumentException) {
            throw e
        } catch (e: Exception) {
            def
        }
    }

    /**
     * Get the value for the given key, and return as an integer.
     * @param key the key to lookup
     * @param def a default value to return
     * @return the key parsed as an integer, or def if the key isn't found or cannot be parsed
     * @throws IllegalArgumentException if the key exceeds 32 characters
     */
    @Throws(IllegalArgumentException::class)
    fun getInt(context: Context, key: String, def: Int): Int {
        return try {
            val cl = context.classLoader
            val sp = cl.loadClass(SYSTEM_PROPERTIES)

            val getInt = sp.getMethod("getInt", String::class.java, Int::class.javaPrimitiveType)
            getInt.invoke(sp, key, def) as Int
        } catch (e: IllegalArgumentException) {
            throw e
        } catch (e: Exception) {
            def
        }
    }

    /**
     * Get the value for the given key, and return as a long.
     * @param key the key to lookup
     * @param def a default value to return
     * @return the key parsed as a long, or def if the key isn't found or cannot be parsed
     * @throws IllegalArgumentException if the key exceeds 32 characters
     */
    @Throws(IllegalArgumentException::class)
    fun getLong(context: Context, key: String, def: Long): Long {
        return try {
            val cl = context.classLoader
            val sp = cl.loadClass(SYSTEM_PROPERTIES)

            val getLong = sp.getMethod("getLong", String::class.java, Long::class.javaPrimitiveType)
            getLong.invoke(sp, key, def) as Long
        } catch (e: IllegalArgumentException) {
            throw e
        } catch (e: Exception) {
            def
        }
    }

    /**
     * Get the value for the given key, returned as a boolean.
     * Values 'n', 'no', '0', 'false' or 'off' are considered false.
     * Values 'y', 'yes', '1', 'true' or 'on' are considered true.
     * (case insensitive).
     * If the key does not exist, or has any other value, then the default result is returned.
     * @param key the key to lookup
     * @param def a default value to return
     * @return the key parsed as a boolean, or def if the key isn't found or is not able to be
     * parsed as a boolean.
     * @throws IllegalArgumentException if the key exceeds 32 characters
     */
    @Throws(IllegalArgumentException::class)
    fun getBoolean(context: Context, key: String, def: Boolean): Boolean {
        return try {
            val cl = context.classLoader
            val sp = cl.loadClass(SYSTEM_PROPERTIES)

            val getBoolean = sp.getMethod(
                    "getBoolean", String::class.java, Boolean::class.javaPrimitiveType)

            getBoolean.invoke(sp, key, def) as Boolean
        } catch (e: IllegalArgumentException) {
            throw e
        } catch (e: Exception) {
            def
        }
    }

    /**
     * Set the value for the given key.
     * @throws IllegalArgumentException if the key exceeds 32 characters
     * @throws IllegalArgumentException if the value exceeds 92 characters
     */
    @Throws(IllegalArgumentException::class)
    operator fun set(context: Context, key: String, `val`: String) {
        try {
            val cl = context.classLoader
            val sp = cl.loadClass(SYSTEM_PROPERTIES)

            val set = sp.getMethod("set", String::class.java, String::class.java)
            set.invoke(sp, key, `val`)
        } catch (e: IllegalArgumentException) {
            throw e
        } catch (e: Exception) {
        }
    }
}