package org.opencv.samples.tutorial2;

import java.io.FileOutputStream;
import java.io.IOException;
import java.text.SimpleDateFormat;
import java.util.Date;

import org.opencv.android.BaseLoaderCallback;
import org.opencv.android.CameraBridgeViewBase.CvCameraViewFrame;
import org.opencv.android.LoaderCallbackInterface;
import org.opencv.android.OpenCVLoader;
import org.opencv.android.Utils;
import org.opencv.core.CvType;
import org.opencv.core.Mat;
import org.opencv.android.CameraBridgeViewBase;
import org.opencv.android.CameraBridgeViewBase.CvCameraViewListener2;
import org.opencv.android.JavaCameraView;
import org.opencv.imgproc.Imgproc;
import org.opencv.samples.tutorial2.Tutorial2View;
import org.opencv.highgui.Highgui;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.Context;
import android.hardware.Camera;
import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.MotionEvent;
import android.view.View;
import android.view.View.OnTouchListener;
import android.view.WindowManager;
import android.widget.*;
import android.graphics.*;
import android.media.AudioManager;
import android.media.MediaPlayer;
import android.media.RingtoneManager;


public class Tutorial2Activity extends Activity implements CvCameraViewListener2,OnTouchListener{
    private static final String    TAG = "OCVSample::Activity";

    private static final int       VIEW_MODE_RGBA     = 0;
    private static final int       VIEW_MODE_GRAY     = 1;
    private static final int       VIEW_MODE_CANNY    = 2;
    private static final int       VIEW_MODE_FEATURES = 5;

    private int                    mViewMode;
    private int					   Height;
    private Mat                    mRgba;
    private Mat                    mIntermediateMat;
    private Mat                    mGray;
    private Mat                    mRgb;
    private Mat                    mBgra;
        
    private Mat d0,d1,d2,d3,d4,d5,d6,d7,d8,d9;
    private ImageView img0,img1,img2,img3,img4,img5,img6,img7,img8,img9;
    private Bitmap b0,b1,b2,b3,b4,b5,b6,b7,b8,b9;
    
    
    private int                    FrameNum=0;
    private boolean                Warning=false;
    
    //private TextView text1;
    public int s;
    public String s1;
    
    private MenuItem               mItemPreviewRGBA;
    private MenuItem               mItemPreviewGray;
    private MenuItem               mItemPreviewCanny;
    private MenuItem               mItemPreviewFeatures;

    private String mPictureFileName;

    private CameraBridgeViewBase   mOpenCvCameraView;

    //private MediaPlayer				mMediaPlayer;
    
    
    private BaseLoaderCallback  mLoaderCallback = new BaseLoaderCallback(this) {
        @Override
        public void onManagerConnected(int status) {
            switch (status) {
                case LoaderCallbackInterface.SUCCESS:
                {
                    Log.i(TAG, "OpenCV loaded successfully");                    
                                                          
                    // Load native library after(!) OpenCV initialization                  
                    System.loadLibrary("mixed_sample");
                    mOpenCvCameraView.enableView();
                    mOpenCvCameraView.setOnTouchListener(Tutorial2Activity.this);

                } break;
                default:
                {
                    super.onManagerConnected(status);
                } break;
            }
        }
    };

    public Tutorial2Activity() {
        Log.i(TAG, "Instantiated new " + this.getClass());
        
    }

    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        Log.i(TAG, "called onCreate");
        super.onCreate(savedInstanceState);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        setContentView(R.layout.tutorial2_surface_view);

        mOpenCvCameraView = (CameraBridgeViewBase) findViewById(R.id.tutorial2_activity_surface_view);
        mOpenCvCameraView.setCvCameraViewListener(this);
        mOpenCvCameraView.setOnTouchListener(Tutorial2Activity.this);
         
        setVolumeControlStream(AudioManager.STREAM_ALARM);
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        Log.i(TAG, "called onCreateOptionsMenu");
        mItemPreviewRGBA = menu.add("原始影像");        
        //mItemPreviewFeatures = menu.add("車道線偵測˙");
        return true;
    }

    @Override
    public void onPause()
    {
        super.onPause();
        if (mOpenCvCameraView != null)
            mOpenCvCameraView.disableView();
    }

    @Override
    public void onResume()
    {
        super.onResume();
        OpenCVLoader.initAsync(OpenCVLoader.OPENCV_VERSION_2_4_5, this, mLoaderCallback);
    }

    public void onDestroy() {
        super.onDestroy();
        if (mOpenCvCameraView != null)
            mOpenCvCameraView.disableView();
    }

    public void onCameraViewStarted(int width, int height) {
        mRgba = new Mat(height, width, CvType.CV_8UC4);
        mBgra = new Mat(height, width, CvType.CV_8UC4);
        mIntermediateMat = new Mat(height, width, CvType.CV_8UC3);
        
        mGray = new Mat(height, width, CvType.CV_8UC1);
        mRgb = new Mat(height, width, CvType.CV_8UC3);
        
        Height = height;
        mViewMode = VIEW_MODE_FEATURES;  //影像
        //s=0;
        
        //mMediaPlayer = new MediaPlayer();

        
        d0 = new Mat(25,25,CvType.CV_8UC4);
        b0 = BitmapFactory.decodeResource(getResources(), R.drawable.d0);        
        Utils.bitmapToMat(b0, d0);
        

    }

    public void onCameraViewStopped() {
        mRgba.release();
        mBgra.release();
        mGray.release();
        mRgb.release();
        
        d0.release();
        
        mIntermediateMat.release();
        FrameNum = 0;
    }

    public Mat onCameraFrame(CvCameraViewFrame inputFrame) {
        final int viewMode = mViewMode;
        int Alert = 0;
        switch (viewMode) {
        case VIEW_MODE_GRAY:
            // input frame has gray scale format
            Imgproc.cvtColor(inputFrame.gray(), mRgba, Imgproc.COLOR_GRAY2RGBA, 4);
            break;
        case VIEW_MODE_RGBA:
            // input frame has RBGA format
            mRgba = inputFrame.rgba();
            break;
        case VIEW_MODE_CANNY:
            // input frame has gray scale format
        	mRgba = inputFrame.rgba();
            mGray = inputFrame.gray();      
            break;
        case VIEW_MODE_FEATURES:
            // input frame has RGBA format
            mRgba = inputFrame.rgba();
            mGray = inputFrame.gray();
            FrameNum++;
            Imgproc.cvtColor(mRgba, mRgb, Imgproc.COLOR_RGBA2RGB);
            //Warning 
            Warning = FindLaneLines(mGray.getNativeObjAddr(), mRgb.getNativeObjAddr(), Height/3, FrameNum, Alert);
            //s = FindLaneLines(mGray.getNativeObjAddr(), mRgb.getNativeObjAddr(), Height/3, FrameNum, Alert);
            		                //d0.getNativeObjAddr())//,d1.getNativeObjAddr(),d2.getNativeObjAddr(),d3.getNativeObjAddr(),d4.getNativeObjAddr(),d5.getNativeObjAddr(),d6.getNativeObjAddr(),d7.getNativeObjAddr(),d8.getNativeObjAddr(),d9.getNativeObjAddr());
            //text1.setText(FindLaneLines(mGray.getNativeObjAddr(), mRgb.getNativeObjAddr(), Height/3, FrameNum, Alert, s1));
            Imgproc.cvtColor(mRgb, mRgba, Imgproc.COLOR_RGB2RGBA);
            break;
        }
        
        if (Warning){
        	
        	//s1="00000000";
        	//text1.setText(s1);
        	
        	/*
        	mMediaPlayer.reset();                    
        	try
        	{        	
        		mMediaPlayer.setDataSource(this,RingtoneManager.getDefaultUri(RingtoneManager.TYPE_NOTIFICATION ));        	
        		mMediaPlayer.prepare();
        		mMediaPlayer.start();
        	}
        	catch(Exception e)
        	{
        	e.printStackTrace();
        	}
        	*/
	        
        }
        return mRgba;
    }


	public boolean onOptionsItemSelected(MenuItem item) {
        Log.i(TAG, "called onOptionsItemSelected; selected item: " + item);

        if (item == mItemPreviewRGBA) {
            mViewMode = VIEW_MODE_RGBA;
        } else if (item == mItemPreviewGray) {
            mViewMode = VIEW_MODE_GRAY;
        } else if (item == mItemPreviewCanny) {
            mViewMode = VIEW_MODE_CANNY;
        } else if (item == mItemPreviewFeatures) {
            mViewMode = VIEW_MODE_FEATURES;
        }

        return true;
    }
    
    @SuppressLint("SimpleDateFormat")
    @Override
    public boolean onTouch(View v, MotionEvent event) {
        Log.i(TAG,"onTouch event");
        //Toast.makeText(this, s+"" , Toast.LENGTH_SHORT).show();
        
        return false;
    }
                                    //mGray.getNativeObjAddr()
    //public native int FindLaneLines(long matAddrGr, long matAddrRgb, int nHorizonY, int FrameNum, int Alert);
    public native boolean FindLaneLines(long matAddrGr, long matAddrRgb, int nHorizonY, int FrameNum, int Alert);

}
