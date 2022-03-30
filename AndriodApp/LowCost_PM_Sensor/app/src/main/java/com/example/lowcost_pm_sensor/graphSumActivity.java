package com.example.lowcost_pm_sensor;

import android.content.Intent;
import android.os.Bundle;

import androidx.appcompat.app.AppCompatActivity;

import com.jjoe64.graphview.GraphView;
import com.jjoe64.graphview.GridLabelRenderer;
import com.jjoe64.graphview.helper.StaticLabelsFormatter;
import com.jjoe64.graphview.series.DataPoint;
import com.jjoe64.graphview.series.LineGraphSeries;

import java.util.ArrayList;
import java.util.Arrays;

public class graphSumActivity extends AppCompatActivity {

    GraphView graph;
    LineGraphSeries<DataPoint> series;
    private ArrayList AvgList;
    private ArrayList TimeList;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_graph_data);

        double y;

        graph = (GraphView) findViewById(R.id.data_graph);
        series = new LineGraphSeries<DataPoint>();

        Intent intent = getIntent();

        AvgList = (ArrayList) intent.getSerializableExtra("AvgList");
        TimeList = (ArrayList) intent.getSerializableExtra("TimeList");

        String[] Times = new String[TimeList.size()];
        for(int i = 0; i<TimeList.size();i++){
            Times[i] = (String) TimeList.get(i);
        }


        System.out.println(AvgList);
        System.out.println(TimeList);

        int x = -1;
        for(int i = 0; i<AvgList.size();i++){
            x = x + 1;
            y = (double) AvgList.get(i);
            series.appendData(new DataPoint(x,y),true,AvgList.size());
        }

        graph.addSeries(series);
        graph.setTitle("Graph for All Readings");
        graph.setTitleTextSize(80);
        graph.getViewport().setXAxisBoundsManual(true);
        graph.getViewport().setMinX(0);
        GridLabelRenderer gridLabel = graph.getGridLabelRenderer();


        gridLabel.setHorizontalAxisTitle("Dates (dd-mm-yyyy)");
        gridLabel.setVerticalAxisTitle("Avg Dust Density (mg/m^3)");


        StaticLabelsFormatter staticLabelsFormatter = new StaticLabelsFormatter(graph);
        staticLabelsFormatter.setHorizontalLabels(Times);
        graph.getGridLabelRenderer().setLabelFormatter(staticLabelsFormatter);
    }


}
