package edu.wpi.hexahedron;


import android.app.Activity;
import android.content.Context;
import android.content.res.Configuration;
import android.graphics.Bitmap;
import android.hardware.Camera;
import android.hardware.Camera.Size;
import android.util.AttributeSet;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.ViewGroup;
import android.widget.ImageView;

import org.opencv.android.Utils;
import org.opencv.core.Core;
import org.opencv.core.CvType;
import org.opencv.core.Mat;
import org.opencv.core.Point;
import org.opencv.core.Scalar;

import java.io.IOException;
import java.util.List;

public class CameraPreview extends SurfaceView implements SurfaceHolder.Callback, Camera.PreviewCallback {
    private static final String TAG = "CameraPreview";
    private Context mContext;
    private SurfaceHolder mHolder;
    private Camera mCamera;

    public CameraPreview(Context context, AttributeSet attributes) {
        super(context, attributes);
        mContext = context;
        mHolder = getHolder();
        mHolder.addCallback(this);
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        if (null == mCamera) {
            mCamera = Camera.open();
        }

        try {
            mCamera.setPreviewDisplay(mHolder);
            mCamera.startPreview();
        } catch (IOException e) {
            mCamera.release();
            mCamera = null;
        }
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        mCamera.stopPreview();

        Camera.Parameters params = mCamera.getParameters();

        boolean portrait = isPortrait();
        if (portrait) {
            mCamera.setDisplayOrientation(90);
        } else {
            mCamera.setDisplayOrientation(0);
        }

        int previewWidth = width;
        int previewHeight = height;
        if (portrait) {
            previewWidth = height;
            previewHeight = width;
        }

        List<Size> sizes = params.getSupportedPreviewSizes();
        int tmpHeight = 0;
        int tmpWidth = 0;
        for (Size size : sizes) {
            if (size.width > previewWidth || size.height > previewHeight) {
                continue;
            }
            if (tmpHeight < size.height) {
                tmpWidth = size.width;
                tmpHeight = size.height;
            }
        }
        previewWidth = tmpWidth;
        previewHeight = tmpHeight;

        params.setPreviewSize(previewWidth, previewHeight);

        float layoutHeight, layoutWidth;
        if (portrait) {
            layoutHeight = previewWidth;
            layoutWidth = previewHeight;
        } else {
            layoutHeight = previewHeight;
            layoutWidth = previewWidth;
        }

        float factor = Math.min(height / layoutHeight, width / layoutWidth);
        ViewGroup.LayoutParams layout = this.getLayoutParams();
        layout.height = (int)(layoutHeight * factor);
        layout.width = (int)(layoutWidth * factor);
        this.setLayoutParams(layout);

        mCamera.setParameters(params);
        mCamera.setPreviewCallback(this);
        mCamera.startPreview();
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        if (null == mCamera) {
            return;
        }

        mCamera.stopPreview();
        mCamera.release();
        mCamera = null;
    }

    @Override
    public void onPreviewFrame(byte[] bytes, Camera camera) {
        Mat m = Mat.zeros(100,400, CvType.CV_8UC3);

        // convert to bitmap:
        Bitmap bm = Bitmap.createBitmap(m.cols(), m.rows(),Bitmap.Config.ARGB_8888);
        Utils.matToBitmap(m, bm);

        // find the imageview and draw it!
        ImageView iv = (ImageView) ((Activity)mContext).findViewById(R.id.overlay);
        iv.setImageBitmap(bm);
    }

    private boolean isPortrait() {
        return mContext.getResources().getConfiguration().orientation
                == Configuration.ORIENTATION_PORTRAIT;
    }
}
