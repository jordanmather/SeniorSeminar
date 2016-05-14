package com.example.jordanmather.sensortest;


import android.app.Activity;
import android.app.DialogFragment;
import android.app.FragmentManager;
import android.app.FragmentTransaction;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.net.wifi.WifiManager;
import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.Toolbar;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;
import android.widget.Toast;

import java.io.IOException;
import java.io.PrintWriter;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.UnknownHostException;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;


public class MainActivity extends AppCompatActivity
        implements SensorEventListener, SensorNotFoundDialogFragment.SensorNotFoundDialogListener{

    public static final String TAG = "MainActivity";
    public static final int REQUEST_ADDRESS = 1;
    public static final int PORT = 4444;

    //define the value for each signal type we're sending out
    public static final int DEVICE_JOYSTICK = 0;
    public static final int DEVICE_LINEAR_ACCELEROMETER = 1;
    public static final int DEVICE_GRAVITY = 2;
    public static final int CONNECTION_CLOSE = 3;

    //sensors
    private SensorManager mSensorManager;
    private Sensor linearAccelerometer;
    private Sensor gravity;

    //joystick-related variables
    private TextView txtX, txtY;

    //wifi-related variables
    private WifiManager wifi;
    private String mServerAddress = "";
    private DatagramSocket socket;
    private PrintWriter out;
    private boolean isConnected = false;
    float[] currJoystickDat;
    float[] currAccelDat;
    float[] currGravityDat;

    ScheduledExecutorService scheduledExecutorService;


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        Toolbar toolbar = (Toolbar) findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);

        scheduledExecutorService = Executors.newScheduledThreadPool(5);
        //zero out the arrays in case they are called before I assign values to them
        currJoystickDat = new float[4];
        currJoystickDat[0] = 0;
        currJoystickDat[1] = 0;
        currJoystickDat[2] = 0;
        currJoystickDat[3] = 0;
        currAccelDat = new float[4];
        currAccelDat[0] = 0;
        currAccelDat[1] = 0;
        currAccelDat[2] = 0;
        currAccelDat[3] = 0;
        currGravityDat = new float[4];
        currGravityDat[0] = 0;
        currGravityDat[1] = 0;
        currGravityDat[2] = 0;
        currGravityDat[3] = 0;

        //schedule a timer to send out messages
        //done this way because otherwise sending the sensor data was blocking the joystick data
        scheduledExecutorService.scheduleAtFixedRate(new Runnable() {
            @Override
            public void run() {
                sendMessage(currJoystickDat);
                sendMessage(currAccelDat);
                sendMessage(currGravityDat);
            }
        }, 0, 10, TimeUnit.MILLISECONDS);

        //lock app to portrait mode
        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_PORTRAIT);


        //set up joystick
        txtX = (TextView)findViewById(R.id.TextViewX);
        txtY = (TextView)findViewById(R.id.TextViewY);
        JoystickView joystick = (JoystickView)findViewById(R.id.joystickView);
        joystick.setOnJoystickMovedListener(_listener);

        //set up sensormanager
        mSensorManager = (SensorManager) getSystemService(Context.SENSOR_SERVICE);

        //check that the needed sensors are avaliable on device
        if (mSensorManager.getDefaultSensor(Sensor.TYPE_ACCELEROMETER) == null){
            // no accelerometer
            showSensorNotFoundDialog();
        }
        if (mSensorManager.getDefaultSensor(Sensor.TYPE_GRAVITY) == null)
        {
            //No gravity sensor
            showSensorNotFoundDialog();
        }

        //create the two sensors we will be using
        linearAccelerometer = mSensorManager.getDefaultSensor(Sensor.TYPE_LINEAR_ACCELERATION);
        gravity = mSensorManager.getDefaultSensor(Sensor.TYPE_GRAVITY);

        //create the wifiButton to enter the address of the server
        final Button wifiButton = (Button) findViewById(R.id.WifiButton);
        wifiButton.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                Log.d(TAG, "WifiButton clicked");
                startWifiConnection();
            }
        });

        //create the disconnectButton to close the thread on the computer
        final Button disconnectButton = (Button) findViewById(R.id.DisconnectButton);
        disconnectButton.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                if(isConnected) {
                    Log.d(TAG, "Disconnecting");
                    //send message to the computer to close its side of the connection
                    scheduledExecutorService.shutdown();
                    float[] exit = {CONNECTION_CLOSE, 0, 0, 0};
                    sendMessage(exit);
                    finish();
                }
            }
        });
    }

    @Override
    public void onStart()
    {
        super.onStart();
        //check for wifi
        wifi = (WifiManager)getSystemService(Context.WIFI_SERVICE);
        if(!wifi.isWifiEnabled())
        {
            Toast.makeText(this, "Please enable wifi", Toast.LENGTH_LONG).show();
        }

    }

    private void startWifiConnection()
    {
        //get address
        Intent connectionIntent = new Intent(this, DeviceListActivity.class);
        startActivityForResult(connectionIntent, REQUEST_ADDRESS);
    }

    //a listener that activates whenever the joystick is moved
    private JoystickMovedListener _listener = new JoystickMovedListener() {

        @Override
        public void OnMoved(int pan, int tilt) {
            txtX.setText(Integer.toString(pan));
            txtY.setText(Integer.toString(tilt));
            currJoystickDat[0] = DEVICE_JOYSTICK;
            currJoystickDat[1] = tilt;
            currJoystickDat[2] = -pan;
            currJoystickDat[3] = 0;
        }

        @Override
        public void OnReleased() {
            currJoystickDat[0] = DEVICE_JOYSTICK;
            currJoystickDat[1] = 0;
            currJoystickDat[2] = 0;
            currJoystickDat[3] = 0;
            txtX.setText("0");
            txtY.setText("0");
        }
    };




    /**
     * call to display error message
     */
    private void showSensorNotFoundDialog()
    {
        FragmentManager fragmentManager = getFragmentManager();
        DialogFragment dialog = new SensorNotFoundDialogFragment();
        FragmentTransaction fragmentTransaction = fragmentManager.beginTransaction();
        dialog.show(fragmentTransaction, "SensorNotFoundDialogFragment");
    }

    /**
     * close the app after the user has confirmed that they read the error
     * @param dialog The dialog that called the method
     */
    public void onDialogPositiveClick(DialogFragment dialog){
    this.finishAffinity();
    }

    private void setupChat(String address)
    {
        Log.d(TAG, "setupChat()");
        mServerAddress = address;
        new Thread(new ClientThread()).start();

    }

    //start the socket in a different thread so as to not block the UI
    class ClientThread implements Runnable{
        @Override
                public void run()
        {
            try{
                socket = new DatagramSocket();
                InetAddress serverAddress = InetAddress.getByName(mServerAddress);
                socket.connect(serverAddress, PORT);
                Log.d(TAG, "Called Connect");
            }catch (UnknownHostException e){
                e.printStackTrace();
            }catch (IOException e) {
                e.printStackTrace();
            }
            if(socket.isConnected())
            {
                isConnected = true;
            }
        }
    }


    /**
     * Sends a message
     * @param messageValues A float array of values to send
     */
    private void sendMessage(float[] messageValues)
    {
        //check that we're connected before attempting to send
        if(!isConnected){
            return;
        }else {

            //Make sure there's something to send.
            if (messageValues.length > 0) {
                //get message bytes and write
                String message = messageValues[0] + "," + messageValues[1]
                        + "," + messageValues[2] + "," + messageValues[3] + ";";
                /*String megaData = "joystick:" + currJoystickDat[1] + "," + currJoystickDat[2] +
                        ";accelerometer:" + currAccelDat[1] + "," + currAccelDat[2] + "," + currAccelDat[3] +
                        ";gravity:" + currGravityDat[1] + "," + currGravityDat[2] + "," + currGravityDat[3] + ";";*/
                Log.d(TAG, message);
                byte[] data = message.getBytes();
                int length = data.length;
                DatagramPacket pack = new DatagramPacket(data, length);
                try{
                    socket.send(pack);
                }
                catch(IOException e)
                {
                    e.printStackTrace();
                    Log.e(TAG, "sendMessage: error sending Datagram packet");
                }
            }
        }
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        switch (requestCode)
        {
            case REQUEST_ADDRESS:
                if(resultCode == Activity.RESULT_OK)
                {
                    String address = data.getExtras().getString(DeviceListActivity.EXTRA_DEVICE_ADDRESS);
                    setupChat(address);
                }
                break;

        }
    }



    @Override
    public final void onAccuracyChanged(Sensor sensor, int accuracy)
    {
        //do something here if the accuracy changes
    }

    /**
     * What to do when the sensors change.
     * This method could use some cleaning up.
     * @param event The SensorEvent that triggered the method call
     */
    @Override
    public final void onSensorChanged(SensorEvent event)
    {
        //linear_acceleration sensor returns three values for each axis
        if(event.sensor == mSensorManager.getDefaultSensor(Sensor.TYPE_LINEAR_ACCELERATION)) {
            currAccelDat[0] = DEVICE_LINEAR_ACCELEROMETER;
            currAccelDat[1] = event.values[0];
            currAccelDat[2] = event.values[1];
            currAccelDat[3] = event.values[2];

            String linearCoordinates = "Linear Accelerometer: \n";
            linearCoordinates += "x axis: " + Float.toString(event.values[0])+"\n";
            linearCoordinates += "y axis: " + Float.toString(event.values[1])+"\n";
            linearCoordinates += "z axis: " + Float.toString(event.values[2]) + "\n";

            ((TextView) findViewById(R.id.Accelerometer)).setText(linearCoordinates);
        }
        else if(event.sensor == mSensorManager.getDefaultSensor(Sensor.TYPE_GRAVITY)) {
            //grab event values and truncate values since we don't need the specifity
            currGravityDat[0] = DEVICE_GRAVITY;
            currGravityDat[1] = (int) event.values[0];
            currGravityDat[2] = (int) event.values[1];
            currGravityDat[3] = (int) event.values[2];

            String gravityCoordinates = "Gravity: \n";
            gravityCoordinates += "x axis: " + Float.toString((int) event.values[0])+"\n";
            gravityCoordinates += "y axis: " + Float.toString((int) event.values[1])+"\n";
            gravityCoordinates += "z axis: " + Float.toString((int) event.values[2]) + "\n";

            ((TextView) findViewById(R.id.Gravity)).setText(gravityCoordinates);
        }


    }

    @Override
    protected void onResume()
    {
        super.onResume();

        //start listening to sensors again
        mSensorManager.registerListener(this, linearAccelerometer, SensorManager.SENSOR_DELAY_NORMAL);
        mSensorManager.registerListener(this, gravity, SensorManager.SENSOR_DELAY_NORMAL);
    }

    @Override
    protected void onPause()
    {
        super.onPause();
        //don't listen to sensors while paused
        mSensorManager.unregisterListener(this);
    }



    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        getMenuInflater().inflate(R.menu.menu_main, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        // Handle action bar item clicks here. The action bar will
        // automatically handle clicks on the Home/Up button, so long
        // as you specify a parent activity in AndroidManifest.xml.
        int id = item.getItemId();

        //noinspection SimplifiableIfStatement
        if (id == R.id.action_settings) {
            return true;
        }

        return super.onOptionsItemSelected(item);
    }


}
