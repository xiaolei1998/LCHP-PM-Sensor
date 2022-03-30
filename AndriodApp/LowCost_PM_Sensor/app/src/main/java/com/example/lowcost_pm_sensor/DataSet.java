package com.example.lowcost_pm_sensor;

import java.io.Serializable;
import java.util.List;

public class DataSet implements Serializable {
    private List<String> Data;
    private List<String> Time;
    private String DatasetName;
    private String Frequency;

    public DataSet(List<String> data, List<String> time, String datasetName, String frequency) {
        Data = data;
        Time = time;
        DatasetName = datasetName;
        Frequency = frequency;
    }

    public DataSet(){}

    public List<String> getData() {
        return Data;
    }

    public void setData(List<String> data) {
        Data = data;
    }

    public List<String> getTime() {
        return Time;
    }

    public void setTime(List<String> time) {
        Time = time;
    }

    public String getDatasetName() {
        return DatasetName;
    }

    public void setDatasetName(String datasetName) {
        DatasetName = datasetName;
    }

    public String getFrequency() {
        return Frequency;
    }

    public void setFrequency(String frequency) {
        Frequency = frequency;
    }

    public double getAvg(){
        double sum = 0;
        for(int i = 0; i<Data.size();i++){
            sum += Double.parseDouble(Data.get(i));
        }
        double Avg = sum/Data.size();

        return Avg;
    }

}
