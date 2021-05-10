package org.arcore.samples.base

import android.content.Intent
import android.os.Bundle
import android.view.Menu
import android.view.MenuItem
import android.view.View
import android.widget.TextView
import android.widget.Toast
import androidx.appcompat.app.ActionBarDrawerToggle
import androidx.appcompat.app.AppCompatActivity
import androidx.appcompat.widget.Toolbar
import androidx.core.view.GravityCompat
import androidx.drawerlayout.widget.DrawerLayout
import com.google.android.material.navigation.NavigationView
import com.google.ar.core.examples.java.augmentedimage.AugmentedImageActivity
import com.google.ar.core.examples.java.cloudanchor.CloudAnchorActivity
import com.google.ar.core.examples.java.computervision.ComputerVisionActivity
import com.google.ar.core.examples.java.helloar.HelloArActivity
import com.google.ar.core.examples.java.sharedcamera.SharedCameraActivity
import info.hannes.github.AppUpdateHelper
import info.hannes.logcat.LogcatActivity
import org.arcore.samples.BuildConfig
import org.arcore.samples.R

abstract class NavigationActivity : AppCompatActivity(), NavigationView.OnNavigationItemSelectedListener {
    override fun onPostCreate(savedInstanceState: Bundle?) {
        super.onPostCreate(savedInstanceState)
        val toolbar = findViewById<Toolbar>(R.id.toolbar)
        setSupportActionBar(toolbar)
        val drawer = findViewById<DrawerLayout>(R.id.drawer_layout)
        val toggle = ActionBarDrawerToggle(
                this, drawer, toolbar, R.string.navigation_drawer_open, R.string.navigation_drawer_close)
        drawer.addDrawerListener(toggle)
        toggle.syncState()
        val navigationView = findViewById<NavigationView>(R.id.nav_view)
        navigationView.setNavigationItemSelectedListener(this)
        val headerLayout = navigationView.getHeaderView(0)
        (headerLayout.findViewById<View>(R.id.textVersion) as TextView).text = BuildConfig.VERSION_NAME
    }

    override fun onBackPressed() {
        val drawer = findViewById<DrawerLayout>(R.id.drawer_layout)
        if (drawer.isDrawerOpen(GravityCompat.START)) {
            drawer.closeDrawer(GravityCompat.START)
        } else {
            super.onBackPressed()
        }
    }

    override fun onNavigationItemSelected(item: MenuItem): Boolean { // Handle navigation view item clicks here.
        val id = item.itemId
        if (id == R.id.nav_hello_ar_java) {
            openActivity(HelloArActivity::class.java)
        } else if (id == R.id.nav_hello_ar_c) {
            openActivity(com.google.ar.core.examples.c.helloar.HelloArActivity::class.java)
        } else if (id == R.id.nav_shared_camera_java) {
            openActivity(SharedCameraActivity::class.java)
        } else if (id == R.id.nav_computervision_java) {
            openActivity(ComputerVisionActivity::class.java)
        } else if (id == R.id.nav_computervision_c) {
            openActivity(com.google.ar.core.examples.c.computervision.ComputerVisionActivity::class.java)
        } else if (id == R.id.nav_cloud_anchor_java) {
            openActivity(CloudAnchorActivity::class.java)
        } else if (id == R.id.nav_augmented_image_java) {
            openActivity(AugmentedImageActivity::class.java)
        } else if (id == R.id.nav_augmented_faces_java) {
            openActivity(com.google.ar.core.examples.java.augmentedfaces.AugmentedFacesActivity::class.java)
        } else if (id == R.id.nav_augmented_image_c) {
            openActivity(com.google.ar.core.examples.c.augmentedimage.AugmentedImageActivity::class.java)
        }
        val drawer = findViewById<DrawerLayout>(R.id.drawer_layout)
        drawer.closeDrawer(GravityCompat.START)
        return true
    }

    protected fun openActivity(clazz: Class<*>) {
        startActivity(Intent(this, clazz))
        if (clazz.isInstance(NavigationActivity::class.java)) {
            finish()
        }
    }

    override fun onCreateOptionsMenu(menu: Menu): Boolean {
        menuInflater.inflate(R.menu.main_menu, menu)
        return true
    }

    override fun onOptionsItemSelected(item: MenuItem): Boolean {
        return when (item.itemId) {
            R.id.action_update -> {
                AppUpdateHelper.checkForNewVersion(
                        this,
                        BuildConfig.GIT_REPOSITORY,
                        BuildConfig.VERSION_NAME,
                        { msg -> Toast.makeText(this, msg, Toast.LENGTH_LONG).show() },
                        force = true
                )
                true
            }
            R.id.action_logcat -> {
                openActivity(LogcatActivity::class.java)
                true
            }
            else -> super.onOptionsItemSelected(item)
        }
    }

    override fun onResume() {
        super.onResume()
        invalidateOptionsMenu()
    }
}