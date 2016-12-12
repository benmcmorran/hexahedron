package edu.wpi.hexahedron;

import android.app.Activity;
import android.content.Intent;
import android.graphics.Bitmap;
import android.media.MediaPlayer;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.util.Log;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.ProgressBar;
import android.widget.TextView;

import org.opencv.android.CameraBridgeViewBase;
import org.opencv.android.JavaCameraView;
import org.opencv.android.Utils;
import org.opencv.core.CvType;
import org.opencv.core.Mat;
import org.opencv.imgproc.Imgproc;

import cs.min2phase.Search;

public class MainActivity extends Activity implements CameraBridgeViewBase.CvCameraViewListener2 {
    private HexahedronCameraView mPreview;
    private Handler mHandler = new Handler();
    private ProgressBar mProgress;
    private int height, width;
    private Mat faces1, faces2;
    private Button button;
    private ImageView flash;
    MediaPlayer mediaPlayer;
    int currentCapture = 1;
    Search search = new Search();


    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary("opencv_java3");
        System.loadLibrary("native-lib");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN);
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        setContentView(R.layout.activity_main);

        faces1 = new Mat(9, 3, CvType.CV_8UC4);
        faces2 = new Mat(9, 3, CvType.CV_8UC4);

        flash = (ImageView) findViewById(R.id.flash);

        mediaPlayer = MediaPlayer.create(this, R.raw.click);

        mProgress = (ProgressBar) findViewById(R.id.progress);

        button = (Button) findViewById(R.id.next);
        button.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                currentCapture++;
                mediaPlayer.start();
                flash();
            }
        });

        mPreview = (HexahedronCameraView) findViewById(R.id.preview);
        mPreview.enableView();
        mPreview.setCvCameraViewListener(this);
    }

    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
    public native String stringFromJNI();

    public native void findCube(long addr, long facesAddr, int height, int width);
    public native String clusterColors(long addr1, long addr2);

    public void updateText(final String str) {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                button.setText(str);
            }
        });
    }

    public void flash() {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                flash.getLayoutParams().width = width;
                flash.getLayoutParams().height = height;
                flash.bringToFront();
                flash.setVisibility(View.VISIBLE);
            }
        });
        mHandler.postDelayed(new Runnable() {
            public void run() {
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        flash.setVisibility(View.INVISIBLE);
                    }
                });
            }
        }, 250);
    }

    @Override
    public void onPause() {
        super.onPause();  // Always call the superclass method first

        // Release the Camera because we don't need it when paused
        // and other activities might need to use it.
        if (mPreview != null) {
            mPreview.disableView();
            mPreview = null;
        }
    }

    @Override
    public void onResume() {
        super.onResume();  // Always call the superclass method first

        // Get the Camera instance as the activity achieves full user focus
        if (mPreview == null) {
            mPreview = (HexahedronCameraView) findViewById(R.id.preview);
            mPreview.enableView();
            mPreview.setCvCameraViewListener(this);
        }
    }

    @Override
    public void onCameraViewStarted(int width, int height) { mPreview.enableFlash(); }

    @Override
    public void onCameraViewStopped() {
        mPreview.disableFlash();
    }

    @Override
    public Mat onCameraFrame(CameraBridgeViewBase.CvCameraViewFrame inputFrame) {
        Mat frame = inputFrame.rgba();
        long frameAddr = frame.getNativeObjAddr();
        height = this.getWindow().getDecorView().getHeight();
        width = this.getWindow().getDecorView().getWidth();
        switch (currentCapture) {
            case 1:
                mProgress.setProgress(0);
                updateText("Take 1st Picture");
                findCube(frameAddr, faces1.getNativeObjAddr(), height, width);
                break;
            case 2:
                mProgress.setProgress(50);
                updateText("Take 2nd Picture");
                findCube(frameAddr, faces2.getNativeObjAddr(), height, width);
                break;
            case 3:
                String config = clusterColors(faces1.getNativeObjAddr(), faces2.getNativeObjAddr());
                String config1 = config.substring(0, 54);
                String config2 = config.substring(54, 108);
                String result;
                String result1 = solveCube(config1);
                String result2 = solveCube(config2);
                String output = "";
                Intent intent = new Intent(this, ResultActivity.class);
                if(result1.toLowerCase().contains("error")) { result = result2;}
                else { result = result1;}

                if(result1.toLowerCase().contains("error") && result2.toLowerCase().contains("error")) { output = "ERROR: Please Recapture Images";}
                else {
                    String[] moves = result.split("\\s+");
                    Integer counter = 1;
                    for (String s : moves) {
                        switch(s.charAt(0)) {
                            case 'U':
                                s = s.replace("U", "Top Face: ");
                                break;
                            case 'F':
                                s = s.replace("F", "Front Face: ");
                                break;
                            case 'R':
                                s = s.replace("R", "Right Face: ");
                                break;
                            case 'L':
                                s = s.replace("L", "Left Face: ");
                                break;
                            case 'B':
                                s = s.replace("B", "Back Face: ");
                                break;
                            case 'D':
                                s = s.replace("D", "Bottom Face: ");
                                break;
                        }
                        if (s.contains("'") || s.contains("2")) {
                            s = s.replace("'", " CCW 90 °");
                            s = s.replace("2", " 180 °");
                        } else {
                            s = s.concat(" CW 90 °");
                        }
                        s = String.valueOf(counter) + ". " + s;
                        output = output + "\n" + s;
                        counter++;
                    }
                }
                intent.putExtra("solve", output);
                startActivity(intent);
                currentCapture++;
                break;
        }
        return frame;
    }

    private String solveCube(String config) {
        return search.solution(config, 21, 1000, 0, 0);
    }
}
