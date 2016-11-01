// Ben McMorran, CS 549 HW 5

#include "opencv2/opencv.hpp"
#include <stdio.h>
#include <vector>
#include <cmath>
#include <numeric>

using namespace cv;
using namespace std;

typedef enum {
	DISPLAY_RAW,
	DISPLAY_SOBEL,
	DISPLAY_HOUGH,
	DISPLAY_LINE,
	DISPLAY_COUNT
} DisplayMode;

// Taken from http://stackoverflow.com/questions/7446126/opencv-2d-line-intersection-helper-function/7448287#7448287
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

Point findCenter(vector<Point> hexagon) {
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
		} else {
			corner = corner2;
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

int main(int argc, char *argv[]) {
	namedWindow("Cube", 1);

	Mat raw = imread("cubesmall.jpg");
	imshow("Cube", raw); waitKey(-1);

	Mat edges;
	cvtColor(raw, edges, CV_BGR2GRAY);
	GaussianBlur(edges, edges, Size(), 1.2);
	Canny(edges, edges, 100, 200);
	imshow("Cube", edges); waitKey(-1);

	vector<Point> edgePoints, hullPoints, approx;
	findNonZero(edges, edgePoints);
	convexHull(edgePoints, hullPoints);
	approxPolyDP(hullPoints, approx, 10, true);
	vector<Point> hex = simplify(approx, 6);

	Mat vanish = raw.clone();
	for (int i = 0; i < 6; i++) {
		line(vanish, hex[i], hex[(i + 1) % 6], Scalar(0, 0, 255), 2, LINE_AA);
	}
	imshow("Cube", vanish); waitKey(-1);

	Point centf = findCenter(hex);
	for (int i = 0; i < 6; i += 2) {
		line(vanish, hex[i], centf, Scalar(0, 255, 0), 2, LINE_AA);
	}
	imshow("Cube", vanish); waitKey(-1);

	int size = 256;
	Mat faces(size, size * 3, CV_8UC3);
	Mat cleanFaces(size, size * 3, CV_8UC3);
	for (int i = 0; i < 6; i += 2) {
		Point2f src[] = { hex[i], hex[i + 1], hex[(i + 2) % 6], centf };
		Point2f dst[] = { Point(0, 0), Point(0, size), Point(size, size), Point(size, 0) };
		Mat m = getPerspectiveTransform(src, dst);

		Mat face;
		warpPerspective(raw, face, m, Size(size, size));
		face.copyTo(faces(Rect(i * size / 2, 0, size, size)));

		for (int x = 0; x < 3; x++) {
			for (int y = 0; y < 3; y++) {
				Rect roi(x * size / 3 + size / 9, y * size / 3 + size / 9, size / 9, size / 9);
				Mat cell = face(roi);
				Scalar color = mean(cell);

				cleanFaces(Rect(i * size / 2 + x * size / 3, y * size / 3, size / 3, size / 3)) = color;
			}
		}
	}

	imshow("Cube", faces); waitKey(-1);
	imshow("Cube", cleanFaces); waitKey(-1);

	{
		for (int i = 0; i < 6; i += 2) {
			Mat arrow = imread("arrow.jpg");
			Point2f src[] = { hex[i], hex[i + 1], hex[(i + 2) % 6], centf };
			Point2f dst[] = { Point(0, 0), Point(0, 512), Point(512, 512), Point(512, 0) };
			Mat m = getPerspectiveTransform(dst, src);

			Mat arrowTrans;
			warpPerspective(arrow, arrowTrans, m, raw.size());
			add(raw, arrowTrans, raw);
		}
		
		imshow("Cube", raw);
		waitKey(-1);
	}
}