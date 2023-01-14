#include <opencv2/opencv.hpp>
#include <chrono>
#include "slic.h"
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
        // inputimage path,color user wants, algortihm, reference points/tap point, output path

        // int r, g, b;
        // char const *hexColor = "#8060c2";
        // std::sscanf(hexColor, "#%02x%02x%02x", &r, &g, &b);

        // platform_log("r,g,b: %d,%d,%d", r, g, b);

        std::string str = "(1,2),(3,4),(5,6),(7,8)";
        std::vector<std::pair<int, int>> coordinates = parse_coordinates(str);
        for (const std::pair<int, int> &coordinate : coordinates)
        {
            std::cout << "(" << coordinate.first << ", " << coordinate.second << ")" << std::endl;

            platform_log("coordinate:%d,%d", coordinate.first, coordinate.second);
        }

        // TXT FILE  READING
        string myText;
        ifstream MyReadFile("open_cv/loo.txt");
        while (getline(MyReadFile, myText))
        {
            ofstream MyFile("open_cv/output.txt");
            // Write to the file
            MyFile << "Files can be tricky, but it is fun enough!";

            platform_log("logged");
            platform_log("%s", myText.c_str());
        }

        // Close the file
        MyReadFile.close();

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
        int hue = 16;     //-15+15
        int min_sat = 30; //-40

        cv::Scalar minHSV = cv::Scalar(hue - 16, min_sat - 40, 20); // bed
        cv::Scalar maxHSV = cv::Scalar(hue + 15, 255, 240);         // bed was +30

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

        Point p1(400, 600); // todo: this will be read fromn txt file

        // FIND THE RELEVANT CONTOUR
        for (size_t i = 0; i < contours.size(); i++)
        {
            int result_poly = pointPolygonTest(contours[i], p1, false);
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
        h.setTo(30, v > 1);
        s.setTo(178, v > 1);
        v = v + 20;
        merge(hsv_vec, result);

        // BACKGROUND

        cvtColor(result, result, COLOR_HSV2BGR);
        cvtColor(result, result2, COLOR_BGR2GRAY);

        threshold(result2, result2, 20, 255, THRESH_BINARY_INV);
        bitwise_and(img, img, background, result2);
        cvtColor(background, background, cv::COLOR_HSV2BGR); // or rgb

        final_image = result + background;

        platform_log("Output Path: %s", outputPath);
        imwrite(outputPath, final_image); // then compare withy img
        platform_log("Image writed again ");
    }
}
