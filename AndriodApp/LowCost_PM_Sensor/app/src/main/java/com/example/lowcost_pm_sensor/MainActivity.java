package com.example.lowcost_pm_sensor;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;

import android.content.Intent;
import android.os.Bundle;
import android.view.MenuItem;
import android.view.View;

import com.google.android.material.bottomnavigation.BottomNavigationView;
import com.google.android.material.navigation.NavigationBarView;
import com.google.firebase.database.FirebaseDatabase;

import java.util.HashMap;

public class MainActivity extends AppCompatActivity {

    BottomNavigationView bottomNavigationView;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);





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
    
}