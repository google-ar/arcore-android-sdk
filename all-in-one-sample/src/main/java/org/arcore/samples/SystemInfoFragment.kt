package org.arcore.samples

import android.os.Build
import android.os.Bundle
import androidx.preference.Preference
import androidx.preference.PreferenceFragmentCompat

class SystemInfoFragment : PreferenceFragmentCompat() {

    override fun onCreatePreferences(savedInstanceState: Bundle?, rootKey: String?) {
        addPreferencesFromResource(R.xml.prefs)
        //		Build.DEVICE
        //		Build.ID
        //		Build.MANUFACTURER
        //		Build.MODEL
        //		Build.PRODUCT
        //		Build.TAGS
        //		Build.TYPE
        //		Build.USER
        preferenceManager.findPreference<Preference>(PREFERENCE_ + "BOARD")?.summary = Build.BOARD
        preferenceManager.findPreference<Preference>(PREFERENCE_ + "BOARD")?.summary = Build.BOARD
        preferenceManager.findPreference<Preference>(PREFERENCE_ + "BRAND")?.summary = Build.BRAND
        preferenceManager.findPreference<Preference>(PREFERENCE_ + "CPU_ABI")?.summary = Build.SUPPORTED_ABIS[0]
        preferenceManager.findPreference<Preference>(PREFERENCE_ + "DISPLAY")?.summary = Build.DISPLAY
        preferenceManager.findPreference<Preference>(PREFERENCE_ + "USER")?.summary = Build.USER
        preferenceManager.findPreference<Preference>(PREFERENCE_ + "ARCore_Version")?.summary = BuildConfig.VERSION_NAME
    }

    companion object {
        private const val PREFERENCE_ = "preference_"
    }
}