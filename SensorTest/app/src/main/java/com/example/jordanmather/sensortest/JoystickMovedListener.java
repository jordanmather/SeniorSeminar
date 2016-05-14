package com.example.jordanmather.sensortest;

/**
 * Created by Jordan Mather on 2/18/2016.
 * Using file found in joystick project
 * by mobileanarchy @ https://code.google.com/archive/p/mobile-anarchy-widgets/
 */
public interface JoystickMovedListener {
    public void OnMoved(int pan, int tilt);
    public void OnReleased();
}
