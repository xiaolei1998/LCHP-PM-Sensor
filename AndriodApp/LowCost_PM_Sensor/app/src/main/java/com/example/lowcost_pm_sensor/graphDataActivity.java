package com.example.lowcost_pm_sensor;

import android.content.Intent;
import android.os.Bundle;
import android.provider.ContactsContract;

import androidx.appcompat.app.AppCompatActivity;

import com.jjoe64.graphview.GraphView;
import com.jjoe64.graphview.GridLabelRenderer;
import com.jjoe64.graphview.series.DataPoint;
import com.jjoe64.graphview.series.LineGraphSeries;

public class graphDataActivity extends AppCompatActivity {

    GraphView graph;
    LineGraphSeries<DataPoint> series;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_graph_data);

        double y;

        graph = (GraphView) findViewById(R.id.data_graph);
        series = new LineGraphSeries<DataPoint>();


        Intent intent = getIntent();
        DataSet dataS = (DataSet) intent.getExtras().getSerializable("DataSet");
        int x = -Integer.parseInt(dataS.getFrequency());
        for(int i = 0; i<dataS.getData().size();i++){
            x = x + Integer.parseInt(dataS.getFrequency());
            y = Double.parseDouble(dataS.getData().get(i));
            series.appendData(new DataPoint(x,y),true,dataS.getData().size());
        }
        graph.addSeries(series);
        graph.setTitle("DataSet: "+ dataS.getDatasetName());
        graph.setTitleTextSize(100);
        graph.getViewport().setXAxisBoundsManual(true);
        graph.getViewport().setMinX(0);
        GridLabelRenderer gridLabel = graph.getGridLabelRenderer();
        gridLabel.setHorizontalAxisTitle("Time (s)");
        gridLabel.setVerticalAxisTitle("Dust Density (mg/m^3)");
    }
}
