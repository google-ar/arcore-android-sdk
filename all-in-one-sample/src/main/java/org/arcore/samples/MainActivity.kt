package org.arcore.samples

import android.os.Bundle
import org.arcore.samples.base.NavigationActivity

class MainActivity : NavigationActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)
        supportFragmentManager
                .beginTransaction()
                .add(R.id.contentInfo, SystemInfoFragment())
                .commit()
    }
}