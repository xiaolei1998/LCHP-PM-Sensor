package com.example.lowcost_pm_sensor;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import java.util.ArrayList;

public class DataSetListAdapter extends ArrayAdapter<DataSet> {
    private ArrayList<DataSet> DataSetArrayList;
    private Context context;

    public DataSetListAdapter(Context context, ArrayList<DataSet> dataSets) {
        super(context, 0, dataSets);
        this.DataSetArrayList = dataSets;
        this.context = context;
    }

    /**
     * Process the view for each list element for habit event list
     * @param position
     * @param convertView
     * @param parent
     * @return
     */
    @NonNull
    @Override
    public View getView(int position, @Nullable View convertView, @NonNull ViewGroup parent) {
        View view = convertView;
        if (view == null) {
            view = LayoutInflater.from(context).inflate(R.layout.content_dataset, parent, false);
        }

        DataSet S = DataSetArrayList.get(position);

        TextView eventTitleView = view.findViewById(R.id.tv_dataset);

        eventTitleView.setText(S.getDatasetName());

        return view;
    }


}
