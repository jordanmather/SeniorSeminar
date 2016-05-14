package com.example.jordanmather.sensortest;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.DialogFragment;
import android.content.DialogInterface;
import android.os.Bundle;

/**
 * Created by Jordan Mather on 2/17/2016.
 */
public class SensorNotFoundDialogFragment extends DialogFragment {
    @Override
    public Dialog onCreateDialog(Bundle savedInstanceState)
    {

        AlertDialog.Builder builder = new AlertDialog.Builder(getActivity());
        builder.setMessage(R.string.dialog_messageSensorNotPresent)
                .setTitle(R.string.dialog_SensorNotPresent)
                .setPositiveButton(R.string.dialog_ok, new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int id) {
                        // User clicked OK button
                        mListener.onDialogPositiveClick(SensorNotFoundDialogFragment.this);

                    }
                });
        return builder.create();
    }

    public interface SensorNotFoundDialogListener
    {
        public void onDialogPositiveClick(DialogFragment dialog);
    }

    SensorNotFoundDialogListener mListener;
    @Override
    public void onAttach(Activity activity)
    {
        super.onAttach(activity);
        //verify that the host implemented the callback interface
        try{
            mListener = (SensorNotFoundDialogListener) activity;
        }
        catch (ClassCastException e)
        {
            //not implemented, throw exception
            throw new ClassCastException(activity.toString()
                    + " must implement SensorNotFoundDialogListener.");
        }
    }
}
