package com.example.jordanmather.sensortest;

import android.app.Activity;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.view.Window;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ListView;
import android.widget.TextView;

import java.util.Set;

/**
 * Created by Jordan Mather on 3/12/2016.
 */
public class DeviceListActivity extends Activity {

    /**
     * tag for log
     */
    private static final String TAG = "DeviceListActivity";

    /**
     * Return Intent extra
     */
    public static String EXTRA_DEVICE_ADDRESS = "device_address";


    @Override
    protected void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);

        //Setup the window
        requestWindowFeature(Window.FEATURE_INDETERMINATE_PROGRESS);
        setContentView(R.layout.activity_device_list);

        //Set result CANCELED in case the user backs out
        setResult(Activity.RESULT_CANCELED);

        //Init the button to perform device discovery
        Button scanButton = (Button) findViewById(R.id.button_connect);
        scanButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                EditText text = (EditText)findViewById(R.id.edit_remote_address);
                String address = text.getText().toString();
                connect(address);
                v.setVisibility(View.GONE);
            }
        });
    }

    @Override
    protected  void onDestroy()
    {
        super.onDestroy();
    }

    /**
     * Send back address to connect to
     */
    private void connect(String address)
    {
        Log.d(TAG, "connect()");

        //Indicate scanning in the title
        setProgressBarIndeterminateVisibility(true);
        setTitle("connecting...");

        Intent intent = new Intent();
        intent.putExtra(EXTRA_DEVICE_ADDRESS, address);
        setResult(Activity.RESULT_OK, intent);
        finish();
    }
}
