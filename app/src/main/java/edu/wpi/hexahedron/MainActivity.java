package edu.wpi.hexahedron;

import android.app.Activity;
import android.os.Bundle;
import android.util.Log;
import android.view.Window;
import android.view.WindowManager;

import org.opencv.android.CameraBridgeViewBase;
import org.opencv.android.JavaCameraView;
import org.opencv.android.OpenCVLoader;
import org.opencv.core.Core;
import org.opencv.core.CvType;
import org.opencv.core.Mat;
import org.opencv.core.MatOfInt;
import org.opencv.core.MatOfPoint;
import org.opencv.core.MatOfPoint2f;
import org.opencv.core.Point;
import org.opencv.core.Scalar;
import org.opencv.core.Size;
import org.opencv.imgproc.Imgproc;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Comparator;
import java.util.List;

public class MainActivity extends Activity implements CameraBridgeViewBase.CvCameraViewListener2 {
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
        JavaCameraView preview = (JavaCameraView) findViewById(R.id.preview);
        preview.setCvCameraViewListener(this);
        preview.enableView();
    }

    @Override
    public void onCameraViewStarted(int width, int height) {

    }

    @Override
    public void onCameraViewStopped() {

    }

    @Override
    public Mat onCameraFrame(CameraBridgeViewBase.CvCameraViewFrame inputFrame) {
        Mat raw = inputFrame.rgba();

        Mat edges = new Mat();
        Imgproc.cvtColor(raw, edges, Imgproc.COLOR_RGBA2GRAY);
        Imgproc.GaussianBlur(edges, edges, new Size(), 2);
        Imgproc.Canny(edges, edges, 10, 200);

        MatOfPoint edgePoints = new MatOfPoint();
        MatOfInt hull = new MatOfInt();
        MatOfPoint2f edgePointsFloat = new MatOfPoint2f();
        MatOfPoint2f approx = new MatOfPoint2f();
        Core.findNonZero(edges, edgePoints);
        if (edgePoints.rows() == 0) return raw;
        Imgproc.convexHull(edgePoints, hull);
        edgePointsFloat = new MatOfPoint2f(edgePoints.toArray());
        Imgproc.approxPolyDP(edgePointsFloat, approx, 10, true);
//        if (approx.rows() < 6) return raw;
//        List<Point> hex = simplify(approx, 6);

//        int[] hullArray = hull.toArray();
//        Point[] pointArray = edgePoints.toArray();
//        for (int i = 0; i < hullArray.length; i++) {
//            Imgproc.line(
//                    raw,
//                    pointArray[hullArray[i]],
//                    pointArray[hullArray[(i + 1) % hullArray.length]],
//                    new Scalar(255, 0, 0),
//                    2
//            );
//        }

        Point[] approxPoints = approx.toArray();
        for (int i = 0; i < approx.rows(); i++) {
            Imgproc.line(
                    raw,
                    approxPoints[i],
                    approxPoints[(i + 1) % approxPoints.length],
                    new Scalar(255, 0, 0),
                    2
            );
        }


//        for (int i = 0; i < hex.size(); i++) {
//            Imgproc.line(
//                    raw,
//                    hex.get(i),
//                    hex.get((i + 1) % hex.size()),
//                    new Scalar(255, 0, 0),
//                    2
//            );
//        }

        return raw;
    }

    private Point add(Point a, Point b) {
        return new Point(a.x + b.x, a.y + b.y);
    }

    private Point sub(Point a, Point b) {
        return new Point(a.x - b.x, a.y - b.y);
    }

    private Point mul(double a, Point b) {
        return new Point(a * b.x, a * b.y);
    }

    private double norm(Point a) {
        return Math.sqrt(a.x * a.x + a.y * a.y);
    }

    // Adapted from http://stackoverflow.com/questions/7446126/opencv-2d-line-intersection-helper-function/7448287#7448287
    private boolean intersection(Point o1, Point p1, Point o2, Point p2, Point r) {
        Point x = sub(o2, o1);
        Point d1 = sub(p1, o1);
        Point d2 = sub(p2, o2);

        double cross = d1.x * d2.y - d1.y * d2.x;
        if (Math.abs(cross) < /*EPS*/1e-8)
            return false;

        double t1 = (x.x * d2.y - x.y * d2.x) / cross;
        Point res = add(o1, mul(t1, d1));
        r.x = res.x;
        r.y = res.y;
        return true;
    }

    List<Point> simplify(MatOfPoint2f shape, int n) {
        Point[] shapePoints = shape.toArray();
        final Point[] p1 = new Point[shapePoints.length];
        final Point[] p2 = new Point[shapePoints.length];
        for (int i = 0; i < shapePoints.length; i++) {
            p1[i] = shapePoints[i];
            p2[i] = shapePoints[(i + 1) % shapePoints.length];
        }

        Integer[] indices = new Integer[shapePoints.length];
        for (int i = 0; i < indices.length; i++) indices[i] = i;
        Arrays.sort(indices, new Comparator<Integer>() {
            @Override
            public int compare(Integer a, Integer b) {
                double result = norm(sub(p1[b], p2[b])) - norm(sub(p1[a], p2[a]));
                return result < 0 ? -1 : result == 0 ? 0 : 1;
            }
        });
        Arrays.sort(indices, 0, n);

        List<Point> result = new ArrayList<>(n);
        for (int i = 1; i <= n; i++) {
            Point l1 = p1[indices[i % n]], l2 = p2[indices[i % n]];
            Point l3 = p1[indices[(i - 1) % n]], l4 = p2[indices[(i - 1) % n]];
            Point corner = new Point();
            intersection(l1, l2, l3, l4, corner);
            result.add(corner);
        }

        return result;
    }
}
