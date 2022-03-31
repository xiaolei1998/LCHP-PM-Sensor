package com.example.lowcost_pm_sensor;

import android.app.Activity;
import android.content.ContentResolver;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.os.PersistableBundle;
import android.provider.DocumentsContract;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;

import androidx.activity.result.ActivityResultCallback;
import androidx.activity.result.ActivityResultLauncher;
import androidx.activity.result.contract.ActivityResultContracts;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;

import com.jjoe64.graphview.GraphView;
import com.jjoe64.graphview.GridLabelRenderer;
import com.jjoe64.graphview.helper.StaticLabelsFormatter;
import com.jjoe64.graphview.series.DataPoint;
import com.jjoe64.graphview.series.LineGraphSeries;

import java.io.BufferedReader;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.util.ArrayList;

public class ReadcsvActivity extends AppCompatActivity {

    private static final int PICK_FILE  = 2;
    private static final int READ_REQUEST_CODE = 2;
    private Button btn;

    private ArrayList<String> TimeList;
    private ArrayList<Double> DataList;

    private InputStream getResources(String s) {
        return null;
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_csv);

        DataList = new ArrayList<Double>();
        TimeList = new ArrayList<String>();
        btn = findViewById(R.id.button_loadCsv);
        btn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
                intent.addCategory(Intent.CATEGORY_OPENABLE);
                intent.setType("text/*");
                startActivityForResult(intent, READ_REQUEST_CODE);
            }
        });

    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, @Nullable Intent data) {
        Uri PathHolder = data.getData();
        super.onActivityResult(requestCode, resultCode, data);
        switch (requestCode) {
            case READ_REQUEST_CODE:
                if (resultCode == RESULT_OK){

                    try {
                        InputStream inputStream = getContentResolver().openInputStream(PathHolder);
                        BufferedReader r = new BufferedReader(new InputStreamReader(inputStream));
                        String mLine;
                        while ((mLine = r.readLine()) != null) {
                            String[] line = mLine.split(",");
                            DataList.add(Double.parseDouble(line[1]));
                            TimeList.add(line[0]);
                        }
                    } catch (FileNotFoundException e) {
                        e.printStackTrace();
                    } catch (IOException e) {
                        e.printStackTrace();
                    }

                }
        }

        GraphView graph = (GraphView) findViewById(R.id.data_graph);
        LineGraphSeries<DataPoint> series = new LineGraphSeries<DataPoint>();

        Intent intent = getIntent();

        String Start = TimeList.get(0);
        String End = TimeList.get(TimeList.size()-1);

        double y;
        int x = -1;
        for(int i = 0; i<DataList.size();i++){
            x = x + 1;
            y = (double) DataList.get(i);
            series.appendData(new DataPoint(x,y),true,DataList.size());
        }

        graph.addSeries(series);
        graph.setTitle("Graph for CSV File");
        graph.setTitleTextSize(80);
        graph.getViewport().setXAxisBoundsManual(true);
        graph.getViewport().setMinX(0);
        GridLabelRenderer gridLabel = graph.getGridLabelRenderer();


        gridLabel.setHorizontalAxisTitle("Dates from " + Start + " to " + End);
        gridLabel.setVerticalAxisTitle("Avg Dust Density (mg/m^3)");



    }



}
