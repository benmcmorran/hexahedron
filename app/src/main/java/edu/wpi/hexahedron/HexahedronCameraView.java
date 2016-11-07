package edu.wpi.hexahedron;

import java.io.FileOutputStream;
import java.util.List;

import org.opencv.android.JavaCameraView;

import android.content.Context;
import android.hardware.Camera;
import android.hardware.Camera.Size;
import android.util.AttributeSet;
import android.util.Log;

public class HexahedronCameraView extends JavaCameraView {

    private static final String TAG = "HexahedronCameraView";

    public HexahedronCameraView(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    public void enableFlash() {
        setFlash(Camera.Parameters.FLASH_MODE_TORCH);
    }

    public void disableFlash() {
        setFlash(Camera.Parameters.FLASH_MODE_OFF);
    }

    private void setFlash(String mode) {
        if (mCamera != null) {
            Camera.Parameters params = mCamera.getParameters();
            params.setFlashMode(mode);
            mCamera.setParameters(params);
        }
    }
}
