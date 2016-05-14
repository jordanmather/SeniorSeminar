package com.example.jordanmather.sensortest;


import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.os.Handler;
import android.util.AttributeSet;
import android.util.Log;
import android.view.MotionEvent;
import android.view.View;

/**
 * Created by Jordan Mather on 2/18/2016.
 * Using file found in Joystick project
 * by mobileanarchy @ https://code.google.com/archive/p/mobile-anarchy-widgets/
 */
public class JoystickView extends View {

    //Private Members
    private Paint circlePaint;
    private Paint handlePaint;
    private int innerPadding;
    private int sensitivity;
    private int handleRadius;
    private int handleInnerBoundaries;
    private JoystickMovedListener listener;
    private double touchX, touchY;
    private final String TAG = "JoystickView";

    //Constructors

    public JoystickView(Context context)
    {
        super(context);
        initJoystickView();
    }

    public JoystickView(Context context, AttributeSet attributeSet)
    {
        super(context, attributeSet);
        initJoystickView();
    }

    public JoystickView(Context context, AttributeSet attributeSet, int defstyle)
    {
        super(context, attributeSet, defstyle);
        initJoystickView();
    }
    //Initialization
    private void initJoystickView()
    {
        setFocusable(true);

        circlePaint = new Paint(Paint.ANTI_ALIAS_FLAG);
        circlePaint.setColor(Color.GRAY);
        circlePaint.setStrokeWidth(1);
        circlePaint.setStyle(Paint.Style.FILL_AND_STROKE);

        handlePaint = new Paint(Paint.ANTI_ALIAS_FLAG);
        handlePaint.setColor(Color.DKGRAY);
        handlePaint.setStrokeWidth(1);
        handlePaint.setStyle(Paint.Style.FILL_AND_STROKE);

        innerPadding = 10;
        sensitivity = -10;
    }

    //Public Methods

    public void setOnJoystickMovedListener(JoystickMovedListener listener)
    {
        this.listener = listener;
    }

    //Drawing Functionality
    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec)
    {
        //make sure we have a circle
        int measuredWidth = measure(widthMeasureSpec);
        int measuredHeight = measure(heightMeasureSpec);
        int d = Math.min(measuredWidth, measuredHeight);

        handleRadius = (int)(d * .25);
        handleInnerBoundaries = handleRadius;

        setMeasuredDimension(d, d);
    }

    private int measure(int measureSpec)
    {
        int result = 0;
        //decode the measurement specifications
        int specMode = MeasureSpec.getMode(measureSpec);
        int specSize = MeasureSpec.getSize(measureSpec);
        if(specMode == MeasureSpec.UNSPECIFIED)
        {
            //return default size of 200 if no bounds are specified
        }
        else
        {
            //we want to fill the entire space, so return max size
            result = specSize;
        }
        return result;
    }

    @Override
    protected void onDraw(Canvas canvas)
    {
        int px = getMeasuredWidth()/2;
        int py = getMeasuredHeight()/2;
        int radius = Math.min(px, py);

        //draw the background
        canvas.drawCircle(px, py, radius - innerPadding, circlePaint);

        //draw the handle
        canvas.drawCircle((int)touchX + px, (int)touchY + py, handleRadius, handlePaint);

        canvas.save();
    }

    @Override
    public boolean onTouchEvent(MotionEvent event)
    {
        int actionType = event.getAction();
        if(actionType == MotionEvent.ACTION_MOVE) {
            int px = getMeasuredWidth() / 2;
            int py = getMeasuredHeight() / 2;
            int radius = Math.min(px, py) - handleInnerBoundaries;

            touchX = (event.getX() - px);
            touchX = Math.max(Math.min(touchX, radius), -radius);

            touchY = (event.getY() - py);
            touchY = Math.max(Math.min(touchY, radius), -radius);

            //coordinates
            Log.d(TAG, "X:" + touchX + "|Y:" + touchY);

            //pressure
            if (listener != null) {
                listener.OnMoved((int) (touchX / radius * sensitivity), (int) (touchY / radius * sensitivity));
            }
            invalidate();
        }
            else if(actionType == MotionEvent.ACTION_UP)
        {
            returnHandleToCenter();
            Log.d(TAG, "X:" + touchX + "|Y:" + touchY);
        }
        return true;
    }

    private void returnHandleToCenter()
    {
        Handler handler = new Handler();
        int numberOfFrames = 5;
        final double intervalsX = (0-touchX)/numberOfFrames;
        final double intervalsY = (0-touchY)/numberOfFrames;

        for(int i = 0; i < numberOfFrames; i++)
        {
            handler.postDelayed(new Runnable() {
                @Override
                public void run() {
                    touchX += intervalsX;
                    touchY += intervalsY;
                    invalidate();
                }
            }, i * 40);
        }

        if(listener != null)
        {
            listener.OnReleased();
        }
    }
}
