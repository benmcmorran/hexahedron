#include <jni.h>
#include <string>
#include <math.h>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <vector>

using namespace std;
using namespace cv;

bool intersection(Point2f o1, Point2f p1, Point2f o2, Point2f p2, Point2f &r) {
    Point2f x = o2 - o1;
    Point2f d1 = p1 - o1;
    Point2f d2 = p2 - o2;

    float cross = d1.x * d2.y - d1.y * d2.x;
    if (abs(cross) < /*EPS*/1e-8)
        return false;

    double t1 = (x.x * d2.y - x.y * d2.x) / cross;
    r = o1 + d1 * t1;
    return true;
}

//
vector<Point> simplify(vector<Point> shape, int n) {
    vector<pair<Point, Point>> segments(shape.size());
    for (int i = 0; i < shape.size(); i++) {
        segments[i].first = shape[i];
        segments[i].second = shape[(i + 1) % shape.size()];
    }

    vector<int> indices(segments.size());
    iota(indices.begin(), indices.end(), 0);
    partial_sort(indices.begin(), indices.begin() + n, indices.end(), [&](int a, int b) -> bool {
        return norm(segments[a].first - segments[a].second) > norm(segments[b].first - segments[b].second);
    });
    sort(indices.begin(), indices.begin() + n);

    vector<Point> result(n);
    for (int i = 1; i <= n; i++) {
        pair<Point, Point> &next = segments[indices[i % n]], &prev = segments[indices[(i - 1) % n]];
        Point2f corner;
        intersection(next.first, next.second, prev.first, prev.second, corner);
        result[i % n] = corner;
    }

    return result;
}

Point findCenter(vector<Point> hexagon, bool &areCornersOdd) {
    vector<Point2f> vanish;
    vector<Point2f> corners;
    Point2f center;

    for (int i = 0; i < 3; i++) {
        Point2f o1 = hexagon[i], //point 1, line 1
                p1 = hexagon[(i + 1) % 6], //point2 line 1
                corner1 = hexagon[(i + 2) % 6],
                o2 = hexagon[(i + 3) % 6],
                p2 = hexagon[(i + 4) % 6],
                corner2 = hexagon[(i + 5) % 6];

        Point2f intr;//vanishing point
        intersection(o1, p1, o2, p2, intr);

        Point2f corner;
        if (norm(intr - corner1) < norm(intr - corner2)) {
            corner = corner1;
            if (i == 0) areCornersOdd = false;
        } else {
            corner = corner2;
            if (i == 0) areCornersOdd = true;
        }

        vanish.push_back(intr);
        corners.push_back(corner);
    }

    vector<Point2f> centers;
    for (int i = 0; i < 3; i++) {
        int next = (i + 1) % 3;

        Point2f centerGuess;
        intersection(vanish[i], corners[i], vanish[next], corners[next], centerGuess);
        centers.push_back(centerGuess);
        center += centerGuess;// store this, do magic
    }

    float threshold = 3;//No idea what this value should be
    float dist1 = sqrt(pow(centers[0].x-centers[1].x,2) + pow(centers[0].y-centers[1].y,2));
    float dist2 = sqrt(pow(centers[1].x-centers[2].x,2) + pow(centers[1].y-centers[2].y,2));
    float dist3 = sqrt(pow(centers[2].x-centers[0].x,2) + pow(centers[2].y-centers[0].y,2));

    if (dist1<threshold && dist2<threshold && dist3<threshold){
        return center / 3;
    }

    return center / 3;//maybe a point with 0,0?
}

class DistanceTest {
public:
    DistanceTest(float threshold) {
        this->threshold = threshold;
    }

    bool operator()(const Point &p1, const Point &p2) {
        return norm(p2 - p1) < threshold;
    }

private:
    float threshold;
};

//function to identify if points belong to the same cluster
vector<Point> findCentralPoints(vector<Vec4i> lines, Point center, int threshold, Mat preview) {
    vector<Point> endpoints;
    for (Vec4i l : lines) {
        endpoints.push_back(Point(l[0], l[1]));
        endpoints.push_back(Point(l[2], l[3]));
    }

    vector<int> labels;
    partition(endpoints, labels, DistanceTest(threshold));

    int maxCols = 6;
    Scalar cols[] = {
            Scalar(255, 0, 0),
            Scalar(255, 255, 0),
            Scalar(0, 255, 0),
            Scalar(0, 255, 255),
            Scalar(0, 0, 255),
            Scalar(255, 0, 255)
    };
    for (int i = 0; i < endpoints.size(); i++) {
        circle(preview, endpoints[i], 5, cols[labels[i] % maxCols], -1, 8, 0);
    }

    int labelCount = *max_element(labels.begin(), labels.end()) + 1;
    vector<pair<int, Point>> averages(labelCount);
    for (int i = 0; i < endpoints.size(); i++) {
        averages[labels[i]].first++;
        averages[labels[i]].second += endpoints[i];
    }
    int centralLabel = min_element(averages.begin(), averages.end(), [&](pair<int, Point> a, pair<int, Point> b) -> bool {
        return norm(a.second / a.first - center) < norm(b.second / b.first - center);
    }) - averages.begin();

    vector<Point> result;
    for (int i = 0; i < endpoints.size(); i++) {
        if (labels[i] == centralLabel) {
            result.push_back(endpoints[i]);
        }
    }

    return result;
}

extern "C" {

jstring
Java_edu_wpi_hexahedron_MainActivity_stringFromJNI(
        JNIEnv *env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}

void
Java_edu_wpi_hexahedron_MainActivity_findCube(
        JNIEnv*,
        jobject,
        jlong input
) {
    Mat& raw = *(Mat*)input; // read image
    Point center(raw.cols / 2, raw.rows / 2); //Cube is close to the center

    Mat edges;
    cvtColor(raw, edges, CV_RGBA2GRAY);
    GaussianBlur(edges, edges, Size(), 1);
    Canny(edges, edges, 100, 200); //find edges

    vector<Vec4i> lines;
    HoughLinesP(edges, lines, 1, CV_PI / 180, 50, 30, 10); //find lines
    if (lines.size() == 0) return;
    vector<Point> cubePoints = findCentralPoints(lines, center, 100, raw); //???

    vector<Point> hullPoints, approx; //create vector of points
    if (cubePoints.size() == 0) return; //if nothing detected, break
    convexHull(cubePoints, hullPoints); //circle the object with a convex shape
    approxPolyDP(hullPoints, approx, 5, true); //approximate the polygon
    if (approx.size() < 6) return; // if sides are less than 6 break
    vector<Point> hex = simplify(approx, 6); //??

    for (int i = 0; i < 6; i++) {
        line(raw, hex[i], hex[(i + 1) % 6], Scalar(0, 0, 255), 2, LINE_AA);//draw the cube
    }

    bool areCornersOdd;
    Point centf = findCenter(hex, areCornersOdd);
    for (int i = areCornersOdd ? 1 : 0; i < 6; i += 2) {
        line(raw, hex[i], centf, Scalar(0, 255, 0), 2, LINE_AA);
    }
}

}
