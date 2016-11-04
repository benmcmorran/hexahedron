package edu.wpi.hexahedron;

import android.app.Activity;
import android.os.Bundle;
import android.util.Log;
import android.view.Window;
import android.view.WindowManager;

import org.opencv.android.OpenCVLoader;

public class MainActivity extends Activity {
    static {
        if (!OpenCVLoader.initDebug()) {
            Log.e("Main", "Couldn't load OpenCV");
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN);
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        setContentView(R.layout.activity_main);
    }
}
