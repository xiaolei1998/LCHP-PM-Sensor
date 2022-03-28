package com.example.lowcost_pm_sensor;

import android.content.Intent;
import android.os.Bundle;
import android.view.MenuItem;
import android.view.View;
import android.widget.Adapter;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.ListView;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;

import com.google.android.material.bottomnavigation.BottomNavigationView;
import com.google.android.material.navigation.NavigationBarView;
import com.google.firebase.auth.FirebaseAuth;
import com.google.firebase.database.DataSnapshot;
import com.google.firebase.database.DatabaseError;
import com.google.firebase.database.DatabaseReference;
import com.google.firebase.database.FirebaseDatabase;
import com.google.firebase.database.ValueEventListener;

import java.util.ArrayList;

public class ViewDataActivity extends AppCompatActivity {

    BottomNavigationView bottomNavigationView;

    ListView DataSetTimeListView;
    ArrayAdapter<String> DataSetTimeAdapter;
    ArrayList<String> DataSetTimeList;


    private FirebaseAuth authentication;
    private String uid; // unique id for each user
    private String Time_Onclick;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_viewdata);


        String Mode = getIntent().getExtras().getString("Mode");
        if (Mode.equals("Offline")){
            getSupportActionBar().setTitle("LowCost_PM_Sensor (Offline)");
        }


        DataSetTimeListView = findViewById(R.id.lv_data_set);

        DataSetTimeList = new ArrayList<>();

        DataSetTimeAdapter = new ArrayAdapter<>(this,R.layout.content,DataSetTimeList);
        DataSetTimeListView.setAdapter(DataSetTimeAdapter);


        if(!Mode.equals("Offline")){
            authentication = FirebaseAuth.getInstance();
            if (authentication.getCurrentUser() != null){
                uid = authentication.getCurrentUser().getUid();
            }
            DatabaseReference reference = FirebaseDatabase.getInstance().getReference().child(uid).child("Datasets");


            reference.addValueEventListener(new ValueEventListener() {
                @Override
                public void onDataChange(@NonNull DataSnapshot snapshot) {
                    DataSetTimeList.clear();
                    for (DataSnapshot dataSnapshot : snapshot.getChildren()){
                        String Times = dataSnapshot.getKey();
                        DataSetTimeList.add(Times);

                    }
                    DataSetTimeAdapter.notifyDataSetChanged();
                }
                @Override
                public void onCancelled(@NonNull DatabaseError error) {
                    System.out.println("Read Data Failed");
                }
            });

            DataSetTimeListView.setOnItemClickListener(new AdapterView.OnItemClickListener() {
                @Override
                public void onItemClick(AdapterView<?> adapterView, View view, int i, long l) {
                    Time_Onclick = DataSetTimeList.get(i).toString();

                    Intent viewData = new Intent(ViewDataActivity.this, DataListActivity.class);
                    viewData.putExtra("Date",Time_Onclick);
                    startActivity(viewData);

                }
            });




        }

























        // Process Navigation Bar
        bottomNavigationView = findViewById(R.id.bottom_navigation_event);
        bottomNavigationView.setSelectedItemId(R.id.navigation_Data);


        bottomNavigationView.setOnItemSelectedListener(new NavigationBarView.OnItemSelectedListener() {
            @Override
            public boolean onNavigationItemSelected(@NonNull MenuItem item) {
                switch (item.getItemId()) {
                    case R.id.navigation_Data:
                        return true;
                    case R.id.navigation_Main:
//                        Intent intent1 = new Intent(ViewDataActivity.this, MainActivity.class);
//                        if (Mode.equals("Offline")){
//                            intent1.putExtra("Mode","Offline");
//                        }else{
//                            intent1.putExtra("Mode","Online");
//                        }
//                        startActivity(intent1);
                        finish();
                        return true;
                    case R.id.navigation_Alert:
                        Intent intent2 = new Intent(ViewDataActivity.this, ProfileActivity.class);
                        if (Mode.equals("Offline")){
                            intent2.putExtra("Mode","Offline");
                        }else{
                            intent2.putExtra("Mode","Online");
                        }
                        startActivity(intent2);
                        finish();
                        return true;
                }
                return false;
            }
        });
    }
}