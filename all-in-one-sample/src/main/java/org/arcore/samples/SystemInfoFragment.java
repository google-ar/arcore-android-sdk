package org.arcore.samples;

import android.annotation.SuppressLint;
import android.os.Build;
import android.os.Bundle;
import android.preference.PreferenceFragment;


public class SystemInfoFragment extends PreferenceFragment {

    private static final String PREFERENCE_ = "preference_";

    @SuppressLint("HardwareIds")
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        // Load the preferences from an XML resource
        addPreferencesFromResource(R.xml.prefs);

        //		Build.DEVICE
        //		Build.ID
        //		Build.MANUFACTURER
        //		Build.MODEL
        //		Build.PRODUCT
        //		Build.TAGS
        //		Build.TYPE
        //		Build.USER

        findPreference(PREFERENCE_ + "BOARD").setSummary(Build.BOARD);
        findPreference(PREFERENCE_ + "BRAND").setSummary(Build.BRAND);
        findPreference(PREFERENCE_ + "CPU_ABI").setSummary(Build.SUPPORTED_ABIS[0]);
        findPreference(PREFERENCE_ + "DISPLAY").setSummary(Build.DISPLAY);
        findPreference(PREFERENCE_ + "USER").setSummary(Build.USER);
        findPreference(PREFERENCE_ + "OpenCV_Version").setSummary(BuildConfig.VERSION_NAME);

    }

}