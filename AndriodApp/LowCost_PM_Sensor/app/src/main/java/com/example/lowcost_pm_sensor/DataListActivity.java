package com.example.lowcost_pm_sensor;

import android.os.Bundle;
import android.provider.ContactsContract;
import android.service.autofill.Dataset;
import android.widget.ArrayAdapter;
import android.widget.ListView;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;

import com.google.android.material.bottomnavigation.BottomNavigationView;
import com.google.firebase.auth.FirebaseAuth;
import com.google.firebase.database.DataSnapshot;
import com.google.firebase.database.DatabaseError;
import com.google.firebase.database.DatabaseReference;
import com.google.firebase.database.FirebaseDatabase;
import com.google.firebase.database.ValueEventListener;

import java.util.ArrayList;
import java.util.Map;

public class DataListActivity extends AppCompatActivity {

    BottomNavigationView bottomNavigationView;

    ListView DataSetListView;
    ArrayAdapter<DataSet> DataSetAdapter;
    ArrayList<DataSet> DataSetList;


    private FirebaseAuth authentication;
    private String uid; // unique id for each user
    private TextView Top_TextView;


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_view_datalists);


        String Date = getIntent().getExtras().getString("Date");
        Top_TextView = findViewById(R.id.Top_TextView);
        Top_TextView.setText("Collected On: "+Date);

        DataSetListView = findViewById(R.id.lv_data_set_list);

        DataSetList = new ArrayList<DataSet>();



        DataSetAdapter = new DataSetListAdapter(this, DataSetList);
        DataSetListView.setAdapter(DataSetAdapter);



        authentication = FirebaseAuth.getInstance();
        if (authentication.getCurrentUser() != null){
            uid = authentication.getCurrentUser().getUid();
        }
        DatabaseReference reference = FirebaseDatabase.getInstance().getReference().child(uid).child("Datasets").child(Date);


        reference.addValueEventListener(new ValueEventListener() {
            @Override
            public void onDataChange(@NonNull DataSnapshot snapshot) {
                DataSetList.clear();
                for (DataSnapshot dataSnapshot : snapshot.getChildren()){

                    for (DataSnapshot snap : dataSnapshot.getChildren()){
                        //Map dataSetMap = (Map) dataSnapshot.getValue();
                        System.out.println("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
                        //System.out.println(dataSetMap.get("Data"));
                        //DataSet dataS = new DataSet(dataSetMap.get("Data"),dataSetMap.get("DataTime"),dataSetMap.get("DatasetName"),dataSetMap.get("Frequency"));
                        DataSet dataS = (DataSet) snap.getValue(DataSet.class);
                        DataSetList.add(dataS);
                    }

                }
                DataSetAdapter.notifyDataSetChanged();
            }
            @Override
            public void onCancelled(@NonNull DatabaseError error) {
                System.out.println("Read Data Failed");
            }
        });

    }
}
