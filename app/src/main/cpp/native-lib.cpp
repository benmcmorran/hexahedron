#include <jni.h>
#include <string>
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
        Point2f o1 = hexagon[i],
                p1 = hexagon[(i + 1) % 6],
                corner1 = hexagon[(i + 2) % 6],
                o2 = hexagon[(i + 3) % 6],
                p2 = hexagon[(i + 4) % 6],
                corner2 = hexagon[(i + 5) % 6];

        Point2f intr;
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

    for (int i = 0; i < 3; i++) {
        int next = (i + 1) % 3;

        Point2f centerGuess;
        intersection(vanish[i], corners[i], vanish[next], corners[next], centerGuess);
        center += centerGuess;
    }

    return center / 3;
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

vector<Point> findCentralPoints(vector<Vec4i> lines, Point center, int threshold, Mat *preview) {
    vector<Point> endpoints;
    for (Vec4i l : lines) {
        endpoints.push_back(Point(l[0], l[1]));
        endpoints.push_back(Point(l[2], l[3]));
    }

    vector<int> labels;
    partition(endpoints, labels, DistanceTest(threshold));

    if (nullptr != preview) {
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
            circle(*preview, endpoints[i], 5, cols[labels[i] % maxCols], -1, 8, 0);
        }
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
        jlong input,
        jlong facesAddr
) {
    bool debug = false;

    Mat &raw = *(Mat *) input;
    Mat &faces = *(Mat *) facesAddr;
    Point center(raw.cols / 2, raw.rows / 2);

    Mat edges;
    cvtColor(raw, edges, CV_RGBA2GRAY);
    GaussianBlur(edges, edges, Size(), 1);
    Canny(edges, edges, 100, 200);

    vector<Vec4i> lines;
    HoughLinesP(edges, lines, 1, CV_PI / 180, 50, 30, 10);
    if (lines.size() == 0) return;
    vector<Point> cubePoints = findCentralPoints(lines, center, 100, debug ? &raw : nullptr);

    vector<Point> hullPoints, approx;
    if (cubePoints.size() == 0) return;
    convexHull(cubePoints, hullPoints);
    approxPolyDP(hullPoints, approx, 5, true);
    if (approx.size() < 6) return;
    vector<Point> hex = simplify(approx, 6);

    if (debug) {
        for (int i = 0; i < 6; i++) {
            line(raw, hex[i], hex[(i + 1) % 6], Scalar(0, 0, 255), 2, LINE_AA);
        }
    }

    bool areCornersOdd;
    Point centf = findCenter(hex, areCornersOdd);

    if (debug) {
        for (int i = areCornersOdd ? 1 : 0; i < 6; i += 2) {
            line(raw, hex[i], centf, Scalar(0, 255, 0), 2, LINE_AA);
        }
    }

    int topIndex = min_element(hex.begin(), hex.end(), [](Point a, Point b) -> bool {
        return a.y < b.y;
    }) - hex.begin();

    int size = 128;
    for (int i = 0; i < 6; i += 2) {
        int offset = topIndex;
        Point2f src[3][4] = {
                {hex[offset], hex[(offset + 1) % 6], centf, hex[(offset + 5) % 6]},
                {hex[(offset + 5) % 6], centf, hex[(offset + 3) % 6], hex[(offset + 4) % 6]},
                {centf, hex[(offset + 1) % 6], hex[(offset + 2) % 6], hex[(offset + 3) % 6]}
        };
        Point2f dst[] = {Point(0, 0), Point(size, 0), Point(size, size), Point(0, size)};
        Mat m = getPerspectiveTransform(src[i / 2], dst);

        Mat face;
        warpPerspective(raw, face, m, Size(size, size));

        for (int x = 0; x < 3; x++) {
            for (int y = 0; y < 3; y++) {
                Rect roi(x * size / 3 + size / 9, y * size / 3 + size / 9, size / 9, size / 9);
                Mat cell = face(roi);

                Scalar color = mean(cell);

                faces.at<Vec4b>(3 * (i / 2) + y, x) = Vec4b(color[0], color[1], color[2], color[3]);
            }
        }

//        face.copyTo(raw(Rect(i * size / 2, 0, size, size)));
    }

    Mat bigFaces;
    resize(faces, bigFaces, Size(), 50.0, 50.0, INTER_NEAREST);
    bigFaces.copyTo(raw(Rect(0, 0, bigFaces.cols, bigFaces.rows)));
}

jstring
Java_edu_wpi_hexahedron_MainActivity_clusterColors(
        JNIEnv* env,
        jobject,
        jlong faces1Addr,
        jlong faces2Addr
) {
    Mat &faces1 = *(Mat *) faces1Addr;
    Mat &faces2 = *(Mat *) faces2Addr;

    Mat colors(3 * 3 * 6, 4, CV_32F);

    for (int i = 0; i < 2; i++) {
        Mat &faces = i == 0 ? faces1 : faces2;
        for (int y = 0; y < 9; y++) {
            for (int x = 0; x < 3; x++) {
                Vec4b color = faces.at<Vec4b>(y, x);
                for (int j = 0; j < 4; j++) {
                    colors.at<float>((i * 9 + y) * 3 + x, j) = (float)color[j];
                }
            }
        }
    }

    Mat centerColors;
    Mat labels;
    kmeans(colors, 6, labels, TermCriteria(CV_TERMCRIT_ITER | CV_TERMCRIT_EPS, 100, 1), 10, KMEANS_PP_CENTERS, centerColors);

    int d = labels.at<int>(4);
    int l = labels.at<int>(13);
    int b = labels.at<int>(22);

    int u = labels.at<int>(31);
    int f = labels.at<int>(40);
    int r = labels.at<int>(49);

    // Order is URFDLB
    int order[] = {
            27, 28, 29,
            30, 31, 32,
            33, 34, 35,

            45, 46, 47,
            48, 49, 50,
            51, 52, 53,

            36, 37, 38,
            39, 40, 41,
            42, 43, 44,

             0,  1,  2,
             3,  4,  5,
             6,  7,  8,

             9, 10, 11,
            12, 13, 14,
            15, 16, 17,

            18, 19, 20,
            21, 22, 23,
            24, 25, 26
    };

    char msg[60] = { 0 };
    for (int i = 0; i < 54; i++) {
        char face = ' ';
        int label = labels.at<int>(order[i]);
        if (u == label) face = 'U';
        else if (r == label) face = 'R';
        else if (f == label) face = 'F';
        else if (d == label) face = 'D';
        else if (l == label) face = 'L';
        else if (b == label) face = 'B';
        msg[i] = face;
    }
    return env->NewStringUTF(msg);
}
}
