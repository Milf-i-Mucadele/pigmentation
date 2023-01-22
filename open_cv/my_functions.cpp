#include <opencv2/opencv.hpp>
#include <chrono>
#include "slic.h"
#include <fstream>
#include <opencv2/imgproc.hpp>
#include <iostream>
#include <fstream>
#include <string>
#include <opencv2/imgcodecs.hpp>
#include <utility>
#include <vector>
#include <regex>

#ifdef __ANDROID__
#include <android/log.h>

#endif

using namespace cv;
using namespace std;

extern "C"
{

    void platform_log(const char *fmt, ...)
    {
        va_list args;
        va_start(args, fmt);
#ifdef __ANDROID__
        __android_log_vprint(ANDROID_LOG_VERBOSE, "FFI Logger: ", fmt, args);
#else
        vprintf(fmt, args);
#endif
        va_end(args);
    }

    __attribute__((visibility("default"))) __attribute__((used))
    const char *
    getOpenCVVersion()
    {
        return CV_VERSION;
    }

    __attribute__((visibility("default"))) __attribute__((used))
    vector<std::pair<int, int>>
    parse_coordinates(const std::string &str)
    {
        std::vector<std::pair<int, int>> coordinates;
        std::regex pattern(R"(\(([-+]?\d+),([-+]?\d+)\))");
        std::smatch match;
        std::string::const_iterator iter = str.cbegin();
        while (std::regex_search(iter, str.cend(), match, pattern))
        {
            coordinates.emplace_back(std::stoi(match[1]), std::stoi(match[2]));
            iter = match.suffix().first;
        }
        return coordinates;
    }

    __attribute__((visibility("default"))) __attribute__((used)) void convertImageToGrayImage(char *inputImagePath, char *outputPath)
    {
        // THE DIMENSIONS OF THE IMAGE
        platform_log("PATH %s: ", inputImagePath);
        cv::Mat img = cv::imread(inputImagePath);
        platform_log("Length row: %d", img.rows);
        platform_log("Length column: %d", img.cols);

        // BGR -> HSV changing part
        cv::Mat HSVImage;
        cvtColor(img, HSVImage, cv::COLOR_BGR2HSV);
        cvtColor(img, img, cv::COLOR_BGR2HSV);
        // BLUR THE SEGMENTED IMAGE  todo maybe this place will come after segmentation
        blur(HSVImage, HSVImage, Size(30, 30));

        // COLOR SEGMENTATION
        cv::Mat hsv = HSVImage.clone();
        int hue = 16;     //-15+15 for door
        int min_sat = 30; //-40
        // int hue = 16;
        // int min_sat = 150;

        cv::Scalar minHSV = cv::Scalar(hue - 16, min_sat - 40, 20); // bed
        cv::Scalar maxHSV = cv::Scalar(hue + 16, 255, 240);         // bed was +30

        cv::Mat maskHSV, resultHSV;
        cv::inRange(hsv, minHSV, maxHSV, maskHSV);
        cv::bitwise_and(hsv, hsv, resultHSV, maskHSV);
        cv::Mat segmented_img;
        segmented_img = resultHSV.clone();

        // FINDIND THE RELATED CONTOURS
        int thresh = 20;
        Mat segmented_img_gray;
        cvtColor(segmented_img, segmented_img_gray, COLOR_BGR2GRAY);
        threshold(segmented_img_gray, segmented_img_gray, thresh, 255, THRESH_BINARY);
        vector<vector<Point>> contours;
        vector<Vec4i> hierarchy;
        findContours(segmented_img_gray, contours, hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE);

        Mat drawing = Mat::zeros(segmented_img_gray.size(), CV_8UC3);
        int poi_cnt;
        Scalar color = Scalar(255, 255, 255);
        Scalar line_Color(0, 255, 0);

        Point p1(730, 575); // todo: this will be read fromn txt file 350,520 for kapi

        // FIND THE RELEVANT CONTOUR
        for (size_t i = 0; i < contours.size(); i++)
        {
            int result_poly = pointPolygonTest(contours[i], p1, false);
            // drawContours(drawing, contours, (int)i, color, -1, LINE_8, hierarchy, 0);
            if (result_poly == 1)
            {
                drawContours(drawing, contours, (int)i, color, -1, LINE_8, hierarchy, 0);
            }
        }

        // PIGMENTATION
        Mat result, result2, background, final_image;

        cvtColor(drawing, drawing, COLOR_BGR2GRAY);
        bitwise_and(img, img, result, drawing);
        cvtColor(result, result, COLOR_BGR2HSV);

        vector<Mat> hsv_vec;
        split(result, hsv_vec); // this is an opencv function

        cv::Mat &h = hsv_vec[0];
        cv::Mat &s = hsv_vec[1];
        cv::Mat &v = hsv_vec[2];

        double minVal;
        double maxVal;
        Point minLoc;
        Point maxLoc;
        minMaxLoc(s, &minVal, &maxVal, &minLoc, &maxLoc);

        h.setTo(30, v > 1);
        // s = 62;
        // s.setTo(142, v > 1);
        s = s / maxVal;
        s = s + 0.2;
        s = s * 178;
        v = v + 20;
        merge(hsv_vec, result);

        // BACKGROUND
        cvtColor(result, result, COLOR_HSV2BGR);
        cvtColor(result, result2, COLOR_BGR2GRAY);

        threshold(result2, result2, 20, 255, THRESH_BINARY_INV);
        bitwise_and(img, img, background, result2);
        cvtColor(background, background, cv::COLOR_HSV2BGR); // or rgb

        final_image = result + background;

        // RETURNING
        platform_log("Output Path: %s", outputPath);
        imwrite(outputPath, final_image); // then compare withy img
        platform_log("Image writed again ");
    }

    __attribute__((visibility("default"))) __attribute__((used)) void water_shed(char *inputImagePath, char *outputPath, char *inputXorImagePath)
    {
        platform_log("***WATERSHED***");
        cv::Mat img0 = cv::imread(inputImagePath), xor_image, result, result_2, final_image;
        cv::Mat img_xor = cv::imread(inputXorImagePath);
        platform_log("%d,%d", img0.rows, img0.cols);
        platform_log("%d,%d", img_xor.rows, img_xor.cols);

        platform_log(inputImagePath);
        platform_log(outputPath);
        platform_log(inputXorImagePath);

        bitwise_xor(img0, img_xor, xor_image);
        cvtColor(xor_image, xor_image, COLOR_BGR2GRAY);
        threshold(xor_image, xor_image, 0, 255, THRESH_BINARY);
        cv::Mat element_ = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(5, 5));
        // erode( wshed, wshed, element );
        // imshow( "erode", wshed );
        dilate(xor_image, xor_image, element_);
        erode(xor_image, xor_image, element_);
        erode(xor_image, xor_image, element_);
        erode(xor_image, xor_image, element_);
        erode(xor_image, xor_image, element_);
        // imwrite(outputPath, xor_image);
        // cout << xor_image;
        int i, j, mini = 999, minj = 999, maxi = -10, maxj = -10, compCount = 0;
        // imshow("1",xor_image);
        for (i = 0; i < xor_image.rows; i++)
            for (j = 0; j < xor_image.cols; j++)
            {
                // cout << xor_image.at<uchar>(i,j);
                if (xor_image.at<uchar>(i, j) == 255)
                {
                    if (i < mini)
                        mini = i;
                    if (i > maxi)
                        maxi = i;
                    if (j < minj)
                        minj = j;
                    if (j > maxj)
                        maxj = j;
                }
            }
        cvtColor(xor_image, xor_image, COLOR_GRAY2BGR);
        platform_log("%d,%d,%d,%d", mini, minj, maxi, maxj);
        // Top Left Corner
        Point p1(minj - 50, mini - 50);

        // Bottom Right Corner
        Point p2(maxj + 50, maxi + 50);
        // Point center(minj, mini);//Declaring the center point
        // circle(xor_image, center,10, Scalar(0,255,0), 3);//Using circle()function to draw the line//
        // Point center2(maxj, maxi);//Declaring the center point
        // circle(xor_image, center2,10, Scalar(0,255,0), 3);//Using circle()function to draw the line//
        rectangle(xor_image, p1, p2, Scalar(255, 255, 255), 2);

        // cout << mini << endl;
        // cout << maxi << endl;
        // cout << minj << endl;
        // cout << maxj << endl;
        cvtColor(xor_image, xor_image, COLOR_BGR2GRAY);
        // imshow("2",xor_image);
        vector<vector<Point>> contours;
        vector<Vec4i> hierarchy;
        findContours(xor_image, contours, hierarchy, RETR_CCOMP, CHAIN_APPROX_SIMPLE);

        Mat markers(xor_image.size(), CV_32S);
        markers = Scalar::all(0);
        int idx = 0;
        for (; idx >= 0; idx = hierarchy[idx][0], compCount++)
            drawContours(markers, contours, idx, Scalar::all(compCount + 1), -1, 8, hierarchy, INT_MAX);

        vector<Vec3b> colorTab;
        for (i = 0; i < compCount; i++)
        {
            int b = i * 20;
            int g = i * 20;
            int r = i * 20;
            colorTab.push_back(Vec3b(b, g, r));
        }
        watershed(img0, markers);

        Mat wshed(markers.size(), CV_8UC3);
        // paint the watershed image
        for (i = 0; i < markers.rows; i++)
            for (j = 0; j < markers.cols; j++)
            {
                int index = markers.at<int>(i, j);
                // platform_log("%d",index);
                if (index == -1)
                    wshed.at<cv::Vec3b>(i, j) = cv::Vec3b(255, 255, 255);
                else if (index <= 0 || index > compCount)
                    wshed.at<cv::Vec3b>(i, j) = cv::Vec3b(0, 0, 0);
                else
                    wshed.at<cv::Vec3b>(i, j) = colorTab[index - 1];
            }
        // wshed = wshed*0.5 + imgGray*0.5;
        // imshow( "watershed1", wshed );
        cvtColor(wshed, wshed, COLOR_BGR2GRAY);
        // cout << compCount;
        threshold(wshed, wshed, (compCount - 2) * 20, 255, THRESH_BINARY_INV);
        // bitwise_and(img0, img0, result, wshed);
        // imshow( "watershed2", wshed );
        cv::Mat element = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(5, 5));
        // erode( wshed, wshed, element );
        // imshow( "erode", wshed );
        dilate(wshed, wshed, element);
        // imshow( "dilate", wshed );
        bitwise_and(img0, img0, result, wshed);
        // cout << wshed;
        // threshold(wshed, wshed, 0, 255, THRESH_BINARY);

        cvtColor(result, result, COLOR_BGR2HSV);

        vector<Mat> hsv_vec;
        split(result, hsv_vec); // this is an opencv function

        cv::Mat &h = hsv_vec[0];
        cv::Mat &s = hsv_vec[1];
        cv::Mat &v = hsv_vec[2];
        h.setTo(21, v > 1);
        // s = 62;
        // s.setTo(62, v > 1);
        double minVal;
        double maxVal;
        Point minLoc;
        Point maxLoc;

        minMaxLoc(s, &minVal, &maxVal, &minLoc, &maxLoc);
        s = s / maxVal;
        s = s + 0.1;
        s = s * 155;
        v = v + 20;
        merge(hsv_vec, result);
        cvtColor(result, result, COLOR_HSV2BGR);
        // imshow( "result", result );
        threshold(wshed, wshed, 0, 255, THRESH_BINARY_INV);
        // imshow( "watershed 1", wshed);
        bitwise_and(img0, img0, result_2, wshed);

        final_image = result + result_2;
        imwrite(outputPath, img_xor);
    }
}
