/*
 * Copyright (C) 2014-2019  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

package com.github.chenxiaolong.dualbootpatcher

import android.content.Intent
import android.content.SharedPreferences
import android.content.res.Configuration
import android.graphics.BitmapFactory
import android.graphics.Point
import android.os.Bundle
import android.os.Handler
import android.view.MenuItem
import android.view.View
import android.widget.ImageView
import android.widget.TextView
import androidx.annotation.IdRes
import androidx.appcompat.app.ActionBarDrawerToggle
import androidx.appcompat.app.AppCompatActivity
import androidx.core.content.edit
import androidx.core.view.GravityCompat
import androidx.drawerlayout.widget.DrawerLayout
import androidx.fragment.app.transaction
import com.github.chenxiaolong.dualbootpatcher.appsharing.AppSharingSettingsActivity
import com.github.chenxiaolong.dualbootpatcher.dialogs.GenericProgressDialog
import com.github.chenxiaolong.dualbootpatcher.freespace.FreeSpaceFragment
import com.github.chenxiaolong.dualbootpatcher.patcher.PatchFileFragment
import com.github.chenxiaolong.dualbootpatcher.settings.RomSettingsActivity
import com.github.chenxiaolong.dualbootpatcher.switcher.SwitcherListFragment
import com.github.chenxiaolong.dualbootpatcher.switcher.SwitcherUtils
import com.google.android.material.navigation.NavigationView
import com.google.android.material.navigation.NavigationView.OnNavigationItemSelectedListener
import com.squareup.picasso.Picasso
import kotlin.concurrent.thread

class MainActivity : AppCompatActivity(), OnNavigationItemSelectedListener {
    private lateinit var prefs: SharedPreferences

    private lateinit var drawerLayout: DrawerLayout
    private lateinit var drawerView: NavigationView
    private lateinit var drawerToggle: ActionBarDrawerToggle

    private var drawerItemSelected: Int = 0

    private var appTitle: Int = 0
    private lateinit var handler: Handler
    private var pending: Runnable? = null

    private var fragment: Int = 0

    public override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.drawer_layout)

        handler = Handler()

        prefs = getSharedPreferences("settings", 0)

        if (savedInstanceState != null) {
            appTitle = savedInstanceState.getInt(EXTRA_TITLE)
        }

        setSupportActionBar(findViewById(R.id.toolbar))

        drawerLayout = findViewById(R.id.drawer_layout)
        drawerToggle = object : ActionBarDrawerToggle(this, drawerLayout,
                R.string.drawer_open, R.string.drawer_close) {
            override fun onDrawerClosed(view: View) {
                super.onDrawerClosed(view)

                if (pending != null) {
                    handler.post(pending)
                    pending = null
                }
            }
        }

        drawerLayout.addDrawerListener(drawerToggle)
        drawerLayout.setDrawerShadow(R.drawable.drawer_shadow, GravityCompat.START)
        val actionBar = supportActionBar
        if (actionBar != null) {
            actionBar.setDisplayHomeAsUpEnabled(true)
            actionBar.setHomeButtonEnabled(true)
        }

        drawerView = findViewById(R.id.left_drawer)
        drawerView.setNavigationItemSelectedListener(this)

        // There's a weird performance issue when the drawer is first opened, no matter if we set
        // the background on the nav header RelativeLayout, set the image on an ImageView, or use
        // Picasso to do either asynchronously. By accident, I noticed that using Picasso's resize()
        // method with any dimensions (even the original) works around the performance issue. Maybe
        // something doesn't like PNGs exported from GIMP?
        val header = drawerView.inflateHeaderView(R.layout.nav_header)
        val navImage = header.findViewById<ImageView>(R.id.nav_header_image)
        val dimensions = BitmapFactory.Options()
        dimensions.inJustDecodeBounds = true
        BitmapFactory.decodeResource(resources, R.drawable.material, dimensions)
        Picasso.get().load(R.drawable.material)
                .resize(dimensions.outWidth, dimensions.outHeight).into(navImage)

        // Set nav drawer header text
        val appName = header.findViewById<TextView>(R.id.nav_header_app_name)
        appName.setText(BuildConfig.APP_NAME_RESOURCE)
        val appVersion = header.findViewById<TextView>(R.id.nav_header_app_version)
        appVersion.text = String.format(getString(R.string.version), BuildConfig.VERSION_NAME)

        // Nav drawer width according to material design guidelines
        // http://www.google.com/design/spec/patterns/navigation-drawer.html
        // https://medium.com/sebs-top-tips/material-navigation-drawer-sizing-558aea1ad266
        val display = windowManager.defaultDisplay
        val size = Point()
        display.getSize(size)

        val params = drawerView.layoutParams
        val toolbarHeight = resources.getDimensionPixelSize(
                R.dimen.abc_action_bar_default_height_material)
        params.width = Math.min(size.x - toolbarHeight, 6 * toolbarHeight)
        drawerView.layoutParams = params

        if (savedInstanceState != null) {
            fragment = savedInstanceState.getInt(EXTRA_FRAGMENT)

            showFragment()

            drawerItemSelected = savedInstanceState.getInt(EXTRA_SELECTED_ITEM)
        } else {
            val initialScreens = resources.getStringArray(
                    R.array.initial_screen_entry_values)
            var initialScreen = prefs.getString("initial_screen", null)
            if (initialScreen == null || !initialScreens.contains(initialScreen)) {
                // Show about screen by default
                initialScreen = INITIAL_SCREEN_ABOUT
                prefs.edit {
                    putString("initial_screen", initialScreen)
                }
            }

            drawerItemSelected = when (initialScreen) {
                INITIAL_SCREEN_ABOUT -> R.id.nav_about
                INITIAL_SCREEN_ROMS -> R.id.nav_roms
                INITIAL_SCREEN_PATCHER -> R.id.nav_patch_zip
                else -> throw IllegalStateException("Invalid initial screen value")
            }
        }

        onDrawerItemClicked(drawerItemSelected, false)

        refreshOptionalItems()

        // Open drawer on first start
        thread(start = true) {
            val isFirstStart = prefs.getBoolean("first_start", true)
            if (isFirstStart) {
                drawerLayout.openDrawer(drawerView)
                prefs.edit {
                    putBoolean("first_start", false)
                }
            }
        }
    }

    public override fun onResume() {
        super.onResume()
        refreshOptionalItems()
    }

    public override fun onSaveInstanceState(savedInstanceState: Bundle) {
        super.onSaveInstanceState(savedInstanceState)
        savedInstanceState.putInt(EXTRA_FRAGMENT, fragment)
        savedInstanceState.putInt(EXTRA_TITLE, appTitle)
        savedInstanceState.putInt(EXTRA_SELECTED_ITEM, drawerItemSelected)
    }

    override fun onPostCreate(savedInstanceState: Bundle?) {
        super.onPostCreate(savedInstanceState)
        drawerToggle.syncState()

        updateTitle()
    }

    override fun onConfigurationChanged(newConfig: Configuration) {
        super.onConfigurationChanged(newConfig)
        drawerToggle.onConfigurationChanged(newConfig)
    }

    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        when (requestCode) {
            REQUEST_SETTINGS_ACTIVITY ->
                // Reload activity for theme changes
                recreate()
            else -> super.onActivityResult(requestCode, resultCode, data)
        }
    }

    override fun onOptionsItemSelected(item: MenuItem): Boolean {
        return drawerToggle.onOptionsItemSelected(item) || super.onOptionsItemSelected(item)
    }

    override fun onNavigationItemSelected(menuItem: MenuItem): Boolean {
        onDrawerItemClicked(menuItem.itemId, true)
        return false
    }

    private fun updateTitle() {
        val actionBar = supportActionBar
        if (actionBar != null) {
            if (appTitle == 0) {
                actionBar.setTitle(BuildConfig.APP_NAME_RESOURCE)
            } else {
                actionBar.setTitle(appTitle)
            }
        }
    }

    private fun isItemAFragment(@IdRes item: Int): Boolean {
        return when (item) {
            R.id.nav_roms,
            R.id.nav_patch_zip,
            R.id.nav_free_space,
            R.id.nav_about -> true
            else -> false
        }
    }

    private fun onDrawerItemClicked(@IdRes item: Int, userClicked: Boolean) {
        if (drawerItemSelected == item && userClicked) {
            drawerLayout.closeDrawer(drawerView)
            return
        }

        if (isItemAFragment(item)) {
            drawerItemSelected = item
            // Animate if user clicked
            hideFragments(userClicked)

            val menu = drawerView.menu
            (0 until menu.size())
                    .map { menu.getItem(it) }
                    .forEach { it.isChecked = it.itemId == item }
        }

        if (drawerLayout.isDrawerOpen(drawerView)) {
            pending = Runnable { performDrawerItemSelection(item) }
        } else {
            performDrawerItemSelection(item)
        }

        drawerLayout.closeDrawer(drawerView)
    }

    private fun performDrawerItemSelection(@IdRes item: Int) {
        when (item) {
            R.id.nav_roms -> {
                fragment = FRAGMENT_ROMS
                showFragment()
            }

            R.id.nav_patch_zip -> {
                fragment = FRAGMENT_PATCH_FILE
                showFragment()
            }

            R.id.nav_free_space -> {
                fragment = FRAGMENT_FREE_SPACE
                showFragment()
            }

            R.id.nav_rom_settings -> {
                val intent = Intent(this, RomSettingsActivity::class.java)
                startActivityForResult(intent, REQUEST_SETTINGS_ACTIVITY)
            }

            R.id.nav_app_sharing -> {
                val intent = Intent(this, AppSharingSettingsActivity::class.java)
                startActivity(intent)
            }

            R.id.nav_reboot -> {
                val builder = GenericProgressDialog.Builder()
                builder.message(R.string.please_wait)
                builder.build().show(supportFragmentManager, PROGRESS_DIALOG_REBOOT)

                SwitcherUtils.reboot(this)
            }

            R.id.nav_about -> {
                fragment = FRAGMENT_ABOUT
                showFragment()
            }

            R.id.nav_exit -> finish()

            else -> throw IllegalStateException("Invalid drawer item")
        }
    }

    private fun hideFragments(animate: Boolean) {
        val fm = supportFragmentManager

        val prevRoms = fm.findFragmentByTag(SwitcherListFragment.FRAGMENT_TAG)
        val prevPatchFile = fm.findFragmentByTag(PatchFileFragment.FRAGMENT_TAG)
        val prevFreeSpace = fm.findFragmentByTag(FreeSpaceFragment.FRAGMENT_TAG)
        val prevAbout = fm.findFragmentByTag(AboutFragment.FRAGMENT_TAG)

        fm.transaction {
            if (animate) {
                setCustomAnimations(0, R.animator.fragment_out)
            }

            if (prevRoms != null) {
                hide(prevRoms)
            }
            if (prevPatchFile != null) {
                hide(prevPatchFile)
            }
            if (prevFreeSpace != null) {
                hide(prevFreeSpace)
            }
            if (prevAbout != null) {
                hide(prevAbout)
            }
        }

        fm.executePendingTransactions()
    }

    private fun showFragment() {
        val fm = supportFragmentManager

        val prevRoms = fm.findFragmentByTag(SwitcherListFragment.FRAGMENT_TAG)
        val prevPatchFile = fm.findFragmentByTag(PatchFileFragment.FRAGMENT_TAG)
        val prevFreeSpace = fm.findFragmentByTag(FreeSpaceFragment.FRAGMENT_TAG)
        val prevAbout = fm.findFragmentByTag(AboutFragment.FRAGMENT_TAG)

        fm.transaction {
            setCustomAnimations(R.animator.fragment_in, 0)

            when (fragment) {
                FRAGMENT_ROMS -> {
                    appTitle = R.string.title_roms
                    updateTitle()

                    if (prevRoms == null) {
                        val f = SwitcherListFragment.newInstance()
                        add(R.id.content_frame, f, SwitcherListFragment.FRAGMENT_TAG)
                    } else {
                        show(prevRoms)
                    }
                }

                FRAGMENT_PATCH_FILE -> {
                    appTitle = R.string.title_patch_zip
                    updateTitle()

                    if (prevPatchFile == null) {
                        val f = PatchFileFragment.newInstance()
                        add(R.id.content_frame, f, PatchFileFragment.FRAGMENT_TAG)
                    } else {
                        show(prevPatchFile)
                    }
                }

                FRAGMENT_FREE_SPACE -> {
                    appTitle = R.string.title_free_space
                    updateTitle()

                    if (prevFreeSpace == null) {
                        val f = FreeSpaceFragment.newInstance()
                        add(R.id.content_frame, f, FreeSpaceFragment.FRAGMENT_TAG)
                    } else {
                        show(prevFreeSpace)
                    }
                }

                FRAGMENT_ABOUT -> {
                    appTitle = BuildConfig.APP_NAME_RESOURCE
                    updateTitle()

                    if (prevAbout == null) {
                        val f = AboutFragment.newInstance()
                        add(R.id.content_frame, f, AboutFragment.FRAGMENT_TAG)
                    } else {
                        show(prevAbout)
                    }
                }

                else -> throw IllegalStateException("Invalid fragment ID")
            }
        }
    }

    fun lockNavigation() {
        drawerLayout.setDrawerLockMode(DrawerLayout.LOCK_MODE_LOCKED_CLOSED)
        val actionBar = supportActionBar
        if (actionBar != null) {
            actionBar.setDisplayHomeAsUpEnabled(false)
            actionBar.setHomeButtonEnabled(false)
        }
    }

    fun unlockNavigation() {
        drawerLayout.setDrawerLockMode(DrawerLayout.LOCK_MODE_UNLOCKED)
        val actionBar = supportActionBar
        if (actionBar != null) {
            actionBar.setDisplayHomeAsUpEnabled(true)
            actionBar.setHomeButtonEnabled(true)
        }
    }

    private fun refreshOptionalItems() {
        val reboot = prefs.getBoolean(PREF_SHOW_REBOOT, true)
        val exit = prefs.getBoolean(PREF_SHOW_EXIT, false)
        showReboot(reboot)
        showExit(exit)
    }

    private fun showReboot(visible: Boolean) {
        val menu = drawerView.menu
        val item = menu.findItem(R.id.nav_reboot)
        if (item != null) {
            item.isVisible = visible
        }
    }

    private fun showExit(visible: Boolean) {
        val menu = drawerView.menu
        val item = menu.findItem(R.id.nav_exit)
        if (item != null) {
            item.isVisible = visible
        }
    }

    companion object {
        private const val EXTRA_SELECTED_ITEM = "selected_item"
        private const val EXTRA_TITLE = "title"
        private const val EXTRA_FRAGMENT = "fragment"

        private const val PREF_SHOW_REBOOT = "show_reboot"
        private const val PREF_SHOW_EXIT = "show_exit"

        private val PROGRESS_DIALOG_REBOOT =
                "${MainActivity::class.java.name}.progress.reboot"

        private const val REQUEST_SETTINGS_ACTIVITY = 1000

        const val FRAGMENT_ROMS = 1
        const val FRAGMENT_PATCH_FILE = 2
        const val FRAGMENT_FREE_SPACE = 3
        const val FRAGMENT_ABOUT = 4

        private const val INITIAL_SCREEN_ABOUT = "ABOUT"
        private const val INITIAL_SCREEN_ROMS = "ROMS"
        private const val INITIAL_SCREEN_PATCHER = "PATCHER"
    }
}
