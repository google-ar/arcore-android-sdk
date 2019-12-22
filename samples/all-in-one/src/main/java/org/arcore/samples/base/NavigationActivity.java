package org.arcore.samples.base;

import android.content.Intent;
import android.os.Bundle;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.ActionBarDrawerToggle;
import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.widget.Toolbar;
import androidx.core.view.GravityCompat;
import androidx.drawerlayout.widget.DrawerLayout;

import com.google.android.material.navigation.NavigationView;

import org.arcore.samples.BuildConfig;
import org.arcore.samples.R;

public abstract class NavigationActivity extends AppCompatActivity implements NavigationView.OnNavigationItemSelectedListener {

    @Override
    protected void onPostCreate(@Nullable Bundle savedInstanceState) {
        super.onPostCreate(savedInstanceState);

        Toolbar toolbar = findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);

        DrawerLayout drawer = findViewById(R.id.drawer_layout);
        ActionBarDrawerToggle toggle = new ActionBarDrawerToggle(
                this, drawer, toolbar, R.string.navigation_drawer_open, R.string.navigation_drawer_close);
        drawer.addDrawerListener(toggle);
        toggle.syncState();
        NavigationView navigationView = findViewById(R.id.nav_view);
        navigationView.setNavigationItemSelectedListener(this);

        View headerLayout = navigationView.getHeaderView(0);
        ((TextView) headerLayout.findViewById(R.id.textVersion)).setText(BuildConfig.VERSION_NAME);
    }

    @Override
    public void onBackPressed() {
        DrawerLayout drawer = findViewById(R.id.drawer_layout);
        if (drawer.isDrawerOpen(GravityCompat.START)) {
            drawer.closeDrawer(GravityCompat.START);
        } else {
            super.onBackPressed();
        }
    }

    @Override
    public boolean onNavigationItemSelected(@NonNull MenuItem item) {
        // Handle navigation view item clicks here.
        int id = item.getItemId();

        if (id == R.id.nav_hello_ar_java) {
            openActivity(com.google.ar.core.examples.java.helloar.HelloArActivity.class);
        } else if (id == R.id.nav_hello_ar_c) {
            openActivity(com.google.ar.core.examples.c.helloar.HelloArActivity.class);
        } else if (id == R.id.nav_shared_camera_java) {
            openActivity(com.google.ar.core.examples.java.sharedcamera.SharedCameraActivity.class);
        } else if (id == R.id.nav_computervision_java) {
            openActivity(com.google.ar.core.examples.java.computervision.ComputerVisionActivity.class);
        } else if (id == R.id.nav_computervision_c) {
            openActivity(com.google.ar.core.examples.c.computervision.ComputerVisionActivity.class);
        } else if (id == R.id.nav_cloud_anchor_java) {
            openActivity(com.google.ar.core.examples.java.cloudanchor.CloudAnchorActivity.class);
        } else if (id == R.id.nav_augmented_image_java) {
            openActivity(com.google.ar.core.examples.java.augmentedimage.AugmentedImageActivity.class);
        } else if (id == R.id.nav_augmented_image_c) {
            openActivity(com.google.ar.core.examples.c.augmentedimage.AugmentedImageActivity.class);
        }

        DrawerLayout drawer = findViewById(R.id.drawer_layout);
        drawer.closeDrawer(GravityCompat.START);
        return true;
    }

    protected void openActivity(Class clazz) {
        startActivity(new Intent(this, clazz));
        if (clazz.isInstance(NavigationActivity.class)) {
            finish();
        }
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        getMenuInflater().inflate(R.menu.main, menu);
        return true;
    }

    @Override
    protected void onResume() {
        super.onResume();
        supportInvalidateOptionsMenu();
    }

}

