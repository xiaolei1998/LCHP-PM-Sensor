package com.example.lowcost_pm_sensor;

import android.content.Intent;
import android.os.Bundle;
import android.view.MenuItem;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;

import com.google.android.material.bottomnavigation.BottomNavigationView;
import com.google.android.material.navigation.NavigationBarView;

public class AlertModeActivity extends AppCompatActivity {

    BottomNavigationView bottomNavigationView;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_alertmode);


        String Mode = getIntent().getExtras().getString("Mode");
        if (Mode.equals("Offline")){
            getSupportActionBar().setTitle("LowCost_PM_Sensor (Offline)");
        }
        // Process Navigation Bar
        bottomNavigationView = findViewById(R.id.bottom_navigation_event);
        bottomNavigationView.setSelectedItemId(R.id.navigation_Alert);


        bottomNavigationView.setOnItemSelectedListener(new NavigationBarView.OnItemSelectedListener() {
            @Override
            public boolean onNavigationItemSelected(@NonNull MenuItem item) {
                switch (item.getItemId()) {
                    case R.id.navigation_Data:
                        Intent intent1 = new Intent(AlertModeActivity.this, ViewDataActivity.class);
                        if (Mode.equals("Offline")){
                            intent1.putExtra("Mode","Offline");
                        }else{
                            intent1.putExtra("Mode","Online");
                        }
                        startActivity(intent1);
                        finish();
                        return true;
                    case R.id.navigation_Main:
                        Intent intent2 = new Intent(AlertModeActivity.this, MainActivity.class);
                        if (Mode.equals("Offline")){
                            intent2.putExtra("Mode","Offline");
                        }else{
                            intent2.putExtra("Mode","Online");
                        }
                        startActivity(intent2);
                        finish();
                        return true;
                    case R.id.navigation_Alert:
                        return true;
                }
                return false;
            }
        });
    }
}