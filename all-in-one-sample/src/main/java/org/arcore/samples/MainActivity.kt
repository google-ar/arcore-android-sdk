package org.arcore.samples

import android.os.Bundle
import org.arcore.samples.base.NavigationActivity
import org.arcore.samples.databinding.ActivityMainBinding

class MainActivity : NavigationActivity() {

    private lateinit var binding: ActivityMainBinding

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivityMainBinding.inflate(layoutInflater)
        val view = binding.root
        setContentView(view)

        supportFragmentManager
            .beginTransaction()
            .add(R.id.contentInfo, SystemInfoFragment())
            .commit()

//        AppUpdateHelper.checkForNewVersion(
//                this,
//                BuildConfig.GIT_REPOSITORY,
//                BuildConfig.VERSION_NAME,
//                { msg -> Toast.makeText(this, msg, Toast.LENGTH_LONG).show() }
//        )
    }
}
