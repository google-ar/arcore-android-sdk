package org.arcore.samples

import android.os.Bundle
import android.widget.Toast
import info.hannes.github.AppUpdateHelper
import org.arcore.samples.base.NavigationActivity

class MainActivity : NavigationActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)
        supportFragmentManager
                .beginTransaction()
                .add(R.id.contentInfo, SystemInfoFragment())
                .commit()

        AppUpdateHelper.checkForNewVersion(
                this,
                BuildConfig.GIT_REPOSITORY,
                BuildConfig.VERSION_NAME,
                { msg -> Toast.makeText(this, msg, Toast.LENGTH_LONG).show() }
        )
    }
}