package org.arcore.samples;

import android.os.Bundle;

import org.arcore.samples.base.NavigationActivity;

public class MainActivity extends NavigationActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        getFragmentManager()
                .beginTransaction()
                .add(R.id.contentInfo, new SystemInfoFragment())
                .commit();
    }

}
