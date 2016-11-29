package edu.wpi.hexahedron;

import android.app.Activity;
import android.graphics.Bitmap;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.TextView;

import org.opencv.android.CameraBridgeViewBase;
import org.opencv.android.JavaCameraView;
import org.opencv.android.Utils;
import org.opencv.core.CvType;
import org.opencv.core.Mat;
import org.opencv.imgproc.Imgproc;

public class MainActivity extends Activity implements CameraBridgeViewBase.CvCameraViewListener2 {
    private HexahedronCameraView mPreview;
    private Mat faces1, faces2;
    int currentCapture = 1;

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

        final Button button = (Button) findViewById(R.id.next);
        button.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                currentCapture++;
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

    public native void findCube(long addr, long facesAddr);
    public native String clusterColors(long addr1, long addr2);

    @Override
    public void onCameraViewStarted(int width, int height) {
        mPreview.enableFlash();
    }

    @Override
    public void onCameraViewStopped() {
        mPreview.disableFlash();
    }

    @Override
    public Mat onCameraFrame(CameraBridgeViewBase.CvCameraViewFrame inputFrame) {
        Mat frame = inputFrame.rgba();
        long frameAddr = frame.getNativeObjAddr();
        switch (currentCapture) {
            case 1:
                findCube(frameAddr, faces1.getNativeObjAddr());
                break;
            case 2:
                findCube(frameAddr, faces2.getNativeObjAddr());
                break;
            case 3:
                String result = clusterColors(faces1.getNativeObjAddr(), faces2.getNativeObjAddr());
                Log.d("hexahedron", result);
                break;
        }
        return frame;
    }
}
