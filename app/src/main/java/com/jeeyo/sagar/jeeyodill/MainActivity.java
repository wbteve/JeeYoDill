package com.jeeyo.sagar.jeeyodill;

import android.app.Activity;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCallback;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattDescriptor;
import android.bluetooth.BluetoothManager;
import android.content.Intent;
import android.opengl.GLSurfaceView;
import android.support.v7.app.ActionBarActivity;
import android.os.Bundle;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.MotionEvent;
import android.view.View;
import android.widget.TextView;

import java.util.Arrays;
import java.util.UUID;


public class MainActivity extends Activity {

    public static final String TAG = "BLE";
    private static final String BLE_DEVICE_NAME = "SimpleBLEPeripheral";

    public static final UUID SIMPLEPROFILE = UUID.fromString("0000fff0-0000-1000-8000-00805f9b34fb");
    public static final UUID SIMPLEPROFILE_CHAR1 = UUID.fromString("0000fff1-0000-1000-8000-00805f9b34fb");
    public static final UUID SIMPLEPROFILE_CHAR2 = UUID.fromString("0000fff2-0000-1000-8000-00805f9b34fb");
    public static final UUID SIMPLEPROFILE_CHAR3 = UUID.fromString("0000fff3-0000-1000-8000-00805f9b34fb");
    public static final UUID SIMPLEPROFILE_CHAR4 = UUID.fromString("0000fff4-0000-1000-8000-00805f9b34fb");
    public static final UUID SIMPLEPROFILE_CHAR5 = UUID.fromString("0000fff5-0000-1000-8000-00805f9b34fb");
    private static final UUID CONFIG_DESCRIPTOR = UUID.fromString("00002902-0000-1000-8000-00805f9b34fb");

    private RendererWrapper mRendererWrapper;
    private GLSurfaceView mGLSurfaceView;

    private boolean mBleInitStatus = false;
    private BluetoothAdapter mBluetoothAdapter;
    private BluetoothDevice mDevice = null;
    private BluetoothGatt mGatt = null;

    BluetoothAdapter.LeScanCallback mLeScanCallback = new BluetoothAdapter.LeScanCallback() {
        @Override
        public void onLeScan(final BluetoothDevice device, int rssi, byte[] scanRecord) {
            if(mDevice == null) {
                Log.v(TAG, "New device: " + device.getName());
                if (device.getName().equals(BLE_DEVICE_NAME)) {
                    mDevice = device;
                    Log.v(TAG, "Found required device");
                    mBluetoothAdapter.stopLeScan(mLeScanCallback);
                    mGatt = mDevice.connectGatt(getApplicationContext(), false, btleGattCallback);
                    Log.v(TAG, "Connecting to the device");
                }
            }
        }
    };

    private final BluetoothGattCallback btleGattCallback = new BluetoothGattCallback() {

        @Override
        public void onCharacteristicChanged(BluetoothGatt gatt, final BluetoothGattCharacteristic characteristic) {
            //Log.e(TAG, "Changed-Length: " + characteristic.getValue().length);
            int[] data = new int[8];
            for(int i = 0; i < 8; i++) {
                data[i] = ((characteristic.getValue()[2*i+1] & 0xFF) << 8) | (characteristic.getValue()[2*i] & 0xFF);
                mRendererWrapper.addDataPoint(data[i]);
            }
            //Log.e(TAG, "Changed-data: " + Arrays.toString(data));
        }

        @Override
        public void onConnectionStateChange(final BluetoothGatt gatt, final int status, final int newState) {
            // this will get called when a device connects or disconnects
            if(newState == BluetoothAdapter.STATE_DISCONNECTED) {
                Log.v(TAG, "BLE device disconnected.");
                mDevice = null;
                if(mGatt != null)
                    mGatt.close();
                mGatt = null;
                if(mBleInitStatus) {
                    // App is still alive. Search for BLE devices again.
                    mBleInitStatus = false;
                    BLEInit();
                }
            }
            if(newState == BluetoothAdapter.STATE_CONNECTED) {
                Log.v(TAG, "Connected to BLE device.");
                gatt.discoverServices();
            }
        }

        @Override
        public void onServicesDiscovered(final BluetoothGatt gatt, final int status) {
            // this will get called after the client initiates a BluetoothGatt.discoverServices() call
            Log.d(TAG, String.valueOf(status));
            for(BluetoothGattCharacteristic characteristic: gatt.getService(SIMPLEPROFILE).getCharacteristics()) {
                Log.d(TAG, characteristic.getUuid().toString());
            }
            BluetoothGattCharacteristic characteristic = gatt.getService(SIMPLEPROFILE).getCharacteristic(SIMPLEPROFILE_CHAR4);
            //gatt.readCharacteristic(characteristic); // works for SIMPLEPROFILE_CHAR5 above.

            //Enable local notifications
            gatt.setCharacteristicNotification(characteristic, true);
            //Enabled remote notifications
            BluetoothGattDescriptor desc = characteristic.getDescriptor(CONFIG_DESCRIPTOR);
            desc.setValue(BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE);
            gatt.writeDescriptor(desc);
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setSystemUiVisilityMode();
        mGLSurfaceView = new GLSurfaceView(this);
        mGLSurfaceView.setEGLContextClientVersion(2);
        mRendererWrapper = new RendererWrapper();
        mGLSurfaceView.setRenderer(mRendererWrapper);
        setContentView(mGLSurfaceView);
        mGLSurfaceView.setOnTouchListener(new View.OnTouchListener() {
            @Override
            public boolean onTouch(View v, MotionEvent event) {
                if(event.getAction() == MotionEvent.ACTION_UP) {
                    mRendererWrapper.allowNewVals = !mRendererWrapper.allowNewVals;
                }
                return true;
            }
        });

        BLEInit();
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        Log.v(TAG, "Returning from BLE enable activity.");
        if(resultCode == RESULT_OK) {
            BLEInit();
        }
    }

    private void BLEInit() {
        if(mBleInitStatus == false) {
            final BluetoothManager bluetoothManager =
                    (BluetoothManager) getSystemService(BLUETOOTH_SERVICE);
            mBluetoothAdapter = bluetoothManager.getAdapter();

            if (mBluetoothAdapter == null || !mBluetoothAdapter.isEnabled()) {
                Intent enableBtIntent = new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE);
                startActivityForResult(enableBtIntent, 0);
                return;
            }
            Log.v(TAG, "Searching for BLE Devices");
            mBluetoothAdapter.startLeScan(mLeScanCallback);
            mBleInitStatus = true;
        }
    }

    private void BLEEnd() {
        mBleInitStatus = false;
        if(mGatt != null) {
            Log.v(TAG, "Disconnecting from GATT");
            mGatt.disconnect();
        }
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);
        if(hasFocus) {
            setSystemUiVisilityMode();
        }
    }

    @Override
    protected void onPause() {
        super.onPause();
        BLEEnd();
        mGLSurfaceView.onPause();
        Log.v(TAG, "Exiting");
    }

    @Override
    protected void onResume() {
        super.onResume();
        mGLSurfaceView.onResume();
        BLEInit();
    }

    private void setSystemUiVisilityMode() {
        View decorView = getWindow().getDecorView();
        if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.KITKAT) {
            decorView.setSystemUiVisibility(
                    View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                            | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                            | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                            | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                            | View.SYSTEM_UI_FLAG_FULLSCREEN
                            | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY);
        } else {
            decorView.setSystemUiVisibility(
                    View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                            | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                            | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                            | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                            | View.SYSTEM_UI_FLAG_FULLSCREEN);
        }
    }
}
