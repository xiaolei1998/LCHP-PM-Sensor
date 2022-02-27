package com.example.lowcost_pm_sensor;

import android.Manifest;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Bundle;
import android.view.MenuItem;
import android.widget.CompoundButton;
import android.widget.Switch;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;

import com.google.android.material.bottomnavigation.BottomNavigationView;
import com.google.android.material.navigation.NavigationBarView;

import java.util.ArrayList;
import java.util.List;


public class MainActivity extends AppCompatActivity {
    BottomNavigationView bottomNavigationView;
    Switch BleSwitch;
    private List<String> mPermissionList = new ArrayList<>();
    private String[] permissions = new String[]{Manifest.permission.ACCESS_FINE_LOCATION,Manifest.permission.ACCESS_COARSE_LOCATION};
    private final int mRequestCode = 200;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        initPermission();


        // Process BLE switch
        BleSwitch = (Switch) findViewById(R.id.BLE_switch);
        BleSwitch.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton bottonView, boolean isChecked) {
                if(isChecked) {
                    Intent intent = new Intent(MainActivity.this, BleActivity.class);
                    startActivity(intent);
                    return;
                }
            }
        });



        // Process Navigation Bar
        bottomNavigationView = findViewById(R.id.bottom_navigation_event);
        bottomNavigationView.setSelectedItemId(R.id.navigation_Main);


        bottomNavigationView.setOnItemSelectedListener(new NavigationBarView.OnItemSelectedListener() {
            @Override
            public boolean onNavigationItemSelected(@NonNull MenuItem item) {
                switch (item.getItemId()) {
                    case R.id.navigation_Data:
                        Intent intent1 = new Intent(MainActivity.this, ViewDataActivity.class);
                        startActivity(intent1);
                        finish();
                        return true;
                    case R.id.navigation_Main:
                        return true;
                    case R.id.navigation_Alert:
                        Intent intent2 = new Intent(MainActivity.this, AlertModeActivity.class);
                        startActivity(intent2);
                        finish();
                        return true;
                }
                return false;
            }
        });
    }
    /**
     * Permission check and request
     */
    private void initPermission() {
        mPermissionList.clear();//Clear waiting Permission
        //Check if permission is given
        for (int i = 0; i < permissions.length; i++) {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M && checkSelfPermission(permissions[i])!= PackageManager.PERMISSION_GRANTED) {
                mPermissionList.add(permissions[i]);//Add permission
            }
        }
        //Request for permission
        if (mPermissionList.size() > 0 && Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {//有权限没有通过，需要申请
            requestPermissions(permissions, mRequestCode);
        }
    }


}