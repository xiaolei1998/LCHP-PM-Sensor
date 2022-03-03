package com.example.lowcost_pm_sensor;

import android.Manifest;
import android.annotation.SuppressLint;
import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCallback;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattDescriptor;
import android.bluetooth.BluetoothGattService;
import android.bluetooth.BluetoothProfile;
import android.bluetooth.le.BluetoothLeScanner;
import android.bluetooth.le.ScanCallback;
import android.bluetooth.le.ScanResult;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.text.method.ScrollingMovementMethod;
import android.util.Log;
import android.view.MenuItem;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;

import com.google.android.material.bottomnavigation.BottomNavigationView;
import com.google.android.material.navigation.NavigationBarView;

import java.util.ArrayList;
import java.util.List;
import java.util.UUID;

public class BleActivity extends AppCompatActivity implements View.OnClickListener{
    String TAG = "BleActivity";

    boolean USE_SPECIFIC_UUID = true;//Use specific UUID
    // 服务标识
    private final UUID mServiceUUID = UUID.fromString("ef12c126-80cf-11ec-a8a3-0242ac120002");
    // 特征标识（读取数据）PM value
    private final UUID mCharacteristicUUID_pm = UUID.fromString("7772e5db-3868-4112-a1a9-f2669d106bf3");
    // 特征标识（发送数据） Frequency
    private final UUID mCharacteristicUUID_freq = UUID.fromString("36612c92-80ea-11ec-a8a3-0242ac120002");
    // 描述标识 -- check with group
    private final UUID mConfigUUID = UUID.fromString("00002902-0000-1000-8000-00805f9b34fb");


    // 数组permissions，存储蓝牙连接需要的权限
    private String[] permissions = new String[]{Manifest.permission.ACCESS_FINE_LOCATION,Manifest.permission.ACCESS_COARSE_LOCATION};
    // 未授予的权限存储到mPerrrmissionList
    private List<String> mPermissionList = new ArrayList<>();
    // 权限请求码
    private final int mRequestCode = 200;


    private BluetoothAdapter mBluetoothAdapter;
    private BluetoothLeScanner mBLEScanner;
    private boolean mScanning;//是否正在搜索
    private Handler mHandler;
    private final int SCAN_TIME = 3000;
    private ArrayList<BluetoothDevice> bluetoothDeviceArrayList = new ArrayList<>();
    int selIndex = 0;

    // 是否正在连接
    private boolean mConnectionState = false;
    private BluetoothGatt mBluetoothGatt = null;

    private ArrayList<BluetoothGattCharacteristic> writeCharacteristicArrayList ;
    private ArrayList<BluetoothGattCharacteristic> readCharacteristicArrayList ;
    private ArrayList<BluetoothGattCharacteristic> notifyCharacteristicArrayList ;

    // 按钮
    private Button btn_ble;
    BottomNavigationView bottomNavigationView;
    private TextView tv_device_name;
    // 接收数据框
    private EditText edit_receive_data;

    private int show_ble = 0;

    private  String device_name = "";

    ProgressDialog waitDialog;
    ProgressDialog cancelDialog;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.ble_connection);
        initPermission();
        initView();

        // 获取BluetoothAdapter
        mBluetoothAdapter = BluetoothAdapter.getDefaultAdapter();
        mHandler = new Handler();
        writeCharacteristicArrayList = new ArrayList<>();
        readCharacteristicArrayList = new ArrayList<>();
        notifyCharacteristicArrayList = new ArrayList<>();

        // Process Navigation Bar

        bottomNavigationView = findViewById(R.id.bottom_navigation_event);
        bottomNavigationView.setSelectedItemId(R.id.navigation_Main);


        bottomNavigationView.setOnItemSelectedListener(new NavigationBarView.OnItemSelectedListener() {
            @Override
            public boolean onNavigationItemSelected(@NonNull MenuItem item) {
                switch (item.getItemId()) {
                    case R.id.navigation_Data:
                        Intent intent1 = new Intent(BleActivity.this, ViewDataActivity.class);
                        startActivity(intent1);
                        finish();
                        return true;
                    case R.id.navigation_Main:
                        Intent intent3 = new Intent(BleActivity.this, MainActivity.class);
                        startActivity(intent3);
                        finish();
                        return true;
                    case R.id.navigation_Alert:
                        Intent intent2 = new Intent(BleActivity.this, AlertModeActivity.class);
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

    /**
     * Initialization
     *    Ble button：R.id.btn_ble
     *    Device name：R.id.tv_device_name
     *    Data receving：R.id.edit_receive_data
     *    Show: Connecting...
     *    Inform: No Device find
     */
    private void initView(){

        show_ble = 0;

        btn_ble = findViewById(R.id.btn_ble);
        btn_ble.setOnClickListener((View.OnClickListener) this);

        tv_device_name = findViewById(R.id.tv_device_name);

        edit_receive_data = findViewById(R.id.edit_receive_data);
        edit_receive_data.setMovementMethod(ScrollingMovementMethod.getInstance());

        waitDialog = new ProgressDialog(this);
        waitDialog.setProgressStyle(ProgressDialog.STYLE_SPINNER);
        waitDialog.setCancelable(false);
        waitDialog.setCanceledOnTouchOutside(false);

        waitDialog.setTitle("Please Wait");
        waitDialog.setMessage("Searching for Device...");

        cancelDialog = new ProgressDialog(this);
        cancelDialog.setProgressStyle(ProgressDialog.STYLE_SPINNER);
        cancelDialog.setCancelable(false);
        cancelDialog.setCanceledOnTouchOutside(false);

        cancelDialog.setTitle("Please Wait");
        cancelDialog.setMessage("Disconnecting");
    }

    /**
     * OnClick
     * @param v
     */
    @SuppressLint("MissingPermission")
    @Override
    public void onClick(View v) {
        Log.d(TAG, "click received");
        if(v.getId() == R.id.btn_ble){
            // If connect, then perform disconnecting
            if (mConnectionState){
                cancelDialog.show();
                mHandler.postDelayed(new Runnable() {
                    @Override
                    public void run() {
                        cancelDialog.dismiss();
                    }
                },1000);
                mConnectionState = false;
                btn_ble.setText("Connect BLE");

                if(mBluetoothGatt!=null){
                    mBluetoothGatt.disconnect();
                }
            }else {
                if (!checkBleDevice(this)){
                    return;
                }
                bluetoothDeviceArrayList.clear();
                Log.d(TAG, "scanDevice begin");
                scanLeDevice(true);
            }
        }
    }

    /**
     * Search for Device
     * mBluetoothAdapter.startLeScan(mLeScanCallback);
     */
    @SuppressLint("MissingPermission")
    private void scanLeDevice(final boolean enable) {
        if (enable) {//true
            waitDialog.setMessage("Searching...");
            waitDialog.show();
            mScanning = true;   // mark current state are scanning

            if (mBLEScanner == null){
                mBLEScanner = mBluetoothAdapter.getBluetoothLeScanner();
            }

            //Start Scanning
            mBLEScanner.startScan(mScanCallback);
            //Stop scanning to safe energy
            mHandler.postDelayed(new Runnable() {
                @Override
                public void run() {
                    //stop scanning and mark current state as not scanning
                    mScanning = false;
                    mBLEScanner.stopScan(mScanCallback);
                    waitDialog.dismiss();
                    if (bluetoothDeviceArrayList.size()>0){
                        showScanDeviceList();
                    }
                }
            }, SCAN_TIME);
        } else {//false
            //mark current state as not scanning
            mScanning = false;
            mBLEScanner.stopScan(mScanCallback);
            waitDialog.dismiss();
        }
    }

    /**
     * Result of scanning
     */
    private ScanCallback mScanCallback = new ScanCallback() {
        @SuppressLint("MissingPermission")
        @Override
        public void onScanResult(int callbackType, ScanResult result) {
            super.onScanResult(callbackType, result);
            //get result from scanned device
            BluetoothDevice device = result.getDevice();
            if (device.getName() == null) {
                return;
            }

            for (int i = 0; i < bluetoothDeviceArrayList.size(); i++) {
                if (device.getAddress().equals(bluetoothDeviceArrayList.get(i).getAddress())) {
                    return;
                }
            }
            bluetoothDeviceArrayList.add(device);
        }

        //return numbers of result
        @Override
        public void onBatchScanResults(List<ScanResult> results) {
            super.onBatchScanResults(results);
        }

        @Override
        public void onScanFailed(int errorCode) {
            super.onScanFailed(errorCode);
        }
    };

    @SuppressLint("MissingPermission")
    private void showScanDeviceList(){
        final String[] deviceNames = new String[bluetoothDeviceArrayList.size()];
        for (int i = 0; i < bluetoothDeviceArrayList.size(); i++) {
            if (bluetoothDeviceArrayList.get(i).getName() == null) {
                deviceNames[i] = "Unknow";
            } else {
                deviceNames[i] = bluetoothDeviceArrayList.get(i).getName();
            }
        }

        new AlertDialog.Builder(this).setTitle("Select Device")
                .setSingleChoiceItems(deviceNames, -1, new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int item) {
                        selIndex = item;
                    }
                }).setPositiveButton("Connect", new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int which) {

                waitDialog.setMessage("Connecting...");
                waitDialog.show();
                connectLeDevice(bluetoothDeviceArrayList.get(selIndex).getAddress());
            }
        }).setNegativeButton("Cancel", new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int which) {
            }
        }).show();
    }


    /**
     *We can use Mac address to connect
     * @param address
     * @return
     */

    @SuppressLint("MissingPermission")
    public boolean connectLeDevice(final String address) {
        Log.d(TAG, "连接" + address);
        if (mBluetoothAdapter == null || address == null) {
            Log.d(TAG,"BluetoothAdapter不能初始化 or 未知 address.");
            return false;
        }

        final BluetoothDevice device = mBluetoothAdapter.getRemoteDevice(address);
        if (device == null) {
            Log.d(TAG, "设备没找到，不能连接");
            return false;
        }

        if(mBluetoothGatt!=null){
            mBluetoothGatt.close();
        }
        readCharacteristicArrayList.clear();
        writeCharacteristicArrayList.clear();
        notifyCharacteristicArrayList.clear();
        mBluetoothGatt = device.connectGatt(this, false, mGattCallback);//真正的连接
        return true;
    }

    /**
     * BluetoothGattCallback can be used to pass connecting status and result
     * here it is used to handle connection status
     */
    private final BluetoothGattCallback mGattCallback = new BluetoothGattCallback() {
        @SuppressLint("MissingPermission")
        @Override
        public void onConnectionStateChange(BluetoothGatt gatt, int status,
                                            int newState) {
            Log.d(TAG, "status" + status+",newSatate"+newState);
            if (newState == BluetoothProfile.STATE_CONNECTED) {//当连接状态发生改变
                if (mBluetoothGatt == gatt){
                    mConnectionState = true;
                    // after connection, try discover service
                    gatt.discoverServices();
                    mHandler.post(new Runnable() {
                        @Override
                        public void run() {
                            btn_ble.setText("Disconnect");
                            device_name = mBluetoothGatt.getDevice().getName();
                            tv_device_name.setText("" + device_name);
                            waitDialog.dismiss();
                        }
                    });
                }else {
                    if (mBluetoothGatt == gatt){
                        mConnectionState = true;
                        gatt.discoverServices();
                        device_name = mBluetoothGatt.getDevice().getName();
                    }
                }

            } else if (newState == BluetoothProfile.STATE_DISCONNECTED) {//while not able to connect
                if (mBluetoothGatt == gatt){
                    mConnectionState = false;
                    if(mBluetoothGatt!=null){
                        mBluetoothGatt.close();
                    }
                    mHandler.post(new Runnable() {
                        @Override
                        public void run() {
                            //未连接
                            waitDialog.dismiss();
                            btn_ble.setText("Connect Device");
                            device_name = "";
                            tv_device_name.setText("");
                        }
                    });
                }else {
                    if (mBluetoothGatt == gatt){
                        mConnectionState = false;
                        if(mBluetoothGatt!=null){
                            mBluetoothGatt.close();
                        }
                        device_name = mBluetoothGatt.getDevice().getName();
                    }

                }

            }
        }


        @Override
        // find new service，the return of mBluetoothGatt.discoverServices()
        // here it handles the pm values that send from device
        public void onServicesDiscovered(BluetoothGatt gatt, int status) {
            if (status == BluetoothGatt.GATT_SUCCESS) {
                if (mBluetoothGatt == gatt) {
                    if (USE_SPECIFIC_UUID) {
                        //obtain specific service uuid
                        BluetoothGattService gattService = mBluetoothGatt.getService(mServiceUUID);
                        //specifiv service cannot be null
                        if (gattService != null) {
                            //obtain characteristic
                            BluetoothGattCharacteristic gattCharacteristic = gattService.getCharacteristic(mCharacteristicUUID_pm);
                            setmBluetoothGattNotification(mBluetoothGatt, gattCharacteristic, true);
                            //获取特定特征成功
                            if (gattCharacteristic != null) {
                                readCharacteristicArrayList.add(gattCharacteristic);
                                writeCharacteristicArrayList.add(gattCharacteristic);
                                notifyCharacteristicArrayList.add(gattCharacteristic);
                            }
                        }
                    }

                }

            } else {
                Log.i(TAG, "onServicesDiscovered received: " + status);
            }
        }

        // Read Char
        @Override
        public void onCharacteristicRead(BluetoothGatt gatt,
                                         BluetoothGattCharacteristic characteristic, int status) {
            if (status == BluetoothGatt.GATT_SUCCESS) {
                final byte[] desData = characteristic.getValue();
                Log.i(TAG,"onCharacteristicRead:"+desData.toString());
            }
        }

        @Override // TODO this is going to be write process for freq
        public void onDescriptorWrite(BluetoothGatt gatt,
                                      BluetoothGattDescriptor descriptor, int status) {
            Log.i(TAG,"onDescriptorWrite");
        }

        @Override
        public void onCharacteristicChanged(BluetoothGatt gatt,
                                            BluetoothGattCharacteristic characteristic) {
            final byte[] desData = characteristic.getValue();
            //Log.i(TAG,"onCharacteristicChanged:"+desData.toString());
            if (mBluetoothGatt == gatt){
                mHandler.post(new Runnable() {
                    @Override
                    public void run() {

                        if (edit_receive_data.getText().length()>100){
                            edit_receive_data.setText("");
                        }
                        edit_receive_data.setText(edit_receive_data.getText()+" "+bytesToHex(desData));
                    }
                });
            }

        }



        @Override
        public void onReadRemoteRssi(BluetoothGatt gatt, int rssi, int status) {
            //mBluetoothGatt.readRemoteRssi()调用得到，rssi即信号强度，做防丢器时可以不断使用此方法得到最新的信号强度，从而得到距离。
        }

        public void onCharacteristicWrite(BluetoothGatt gatt,BluetoothGattCharacteristic characteristic, int status) {

            System.out.println("--------write success----- status:" + status);
        }
    };

    /**
     * Set BLE notification if data are received
     */
    @SuppressLint("MissingPermission")
    private boolean setmBluetoothGattNotification(BluetoothGatt bluetoothGatt, BluetoothGattCharacteristic characteristic, boolean enable){
        //Logger.d("setCharacteristicNotification");
        System.out.println("set---------test");
        bluetoothGatt.setCharacteristicNotification(characteristic, enable);
        BluetoothGattDescriptor descriptor = characteristic.getDescriptor(mConfigUUID);
        descriptor.setValue(enable ? BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE : new byte[]{0x00, 0x00});
        return bluetoothGatt.writeDescriptor(descriptor); //descriptor write operation successfully started?
    }

    /**
     * Check if phone support BLE
     */
    public boolean checkBleDevice(Context context) {
        if (mBluetoothAdapter != null) {
            if (!mBluetoothAdapter.isEnabled()) {
                @SuppressLint("MissingPermission") boolean enable = mBluetoothAdapter.enable();
                if (enable) {
                    Toast.makeText(context, "Successfully Open Bluetooth", Toast.LENGTH_SHORT).show();
                    return true;
                } else {
                    Toast.makeText(context, "Failed to open Bluetooth, please open Bluetooth from System setting", Toast.LENGTH_SHORT).show();
                    return false;
                }
            } else {
                return true;
            }
        } else {
            Toast.makeText(context, "Your phone doesn't support Bluetoooth", Toast.LENGTH_SHORT).show();
            return false;

        }
    }

    public static String bytesToHex(byte[] bytes) {
        char[] hexChars = new char[bytes.length * 2];
        char[] hexArray = "0123456789ABCDEF".toCharArray();
        for (int j = 0; j < bytes.length; j++) {
            int v = bytes[j] & 0xFF;
            hexChars[j * 2] = hexArray[v >>> 4];
            hexChars[j * 2 + 1] = hexArray[v & 0x0F];
        }
        return new String(hexChars);
    }

    @Override
    public void onBackPressed() {
        Intent intent3 = new Intent(BleActivity.this, MainActivity.class);
        startActivity(intent3);
        finish();
    }




}
