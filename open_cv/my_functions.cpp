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
    parse_coordinates(const std::string &s)
    {
        std::vector<std::pair<int, int>> coordinates;
        std::regex pattern(R"(\(([-+]?\d+),([-+]?\d+)\))");
        std::smatch match;
        std::string::const_iterator iter = s.cbegin();
        while (std::regex_search(iter, s.cend(), match, pattern))
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

        std::string s = "(1,2),(3,4),(5,6),(7,8)";
        std::vector<std::pair<int, int>> coordinates = parse_coordinates(s);
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
        cv::Mat original = HSVImage.clone();
        cv::Mat clone;
        clone = HSVImage.clone();
        cvtColor(img, HSVImage, cv::COLOR_BGR2HSV);
        cvtColor(img, img, cv::COLOR_BGR2HSV);

        // CHANGING HUE VALUE
        cv::Mat hsv = HSVImage.clone();
        int hue = 16;                                          //+-15
        int min_sat = 100;                                     //-%30
        cv::Scalar minHSV = cv::Scalar(hue - 16, min_sat, 20); // bed

        cv::Scalar maxHSV = cv::Scalar(hue + 15, 255, 240); // bed

        cv::Mat maskHSV, resultHSV;
        cv::inRange(hsv, minHSV, maxHSV, maskHSV);
        cv::bitwise_and(hsv, hsv, resultHSV, maskHSV);
        cv::Mat segmented_img;
        segmented_img = resultHSV.clone();

        // SLIC SUPERPIXELS
        Mat result;
        Mat Blur_img;
        Mat greyimg;

        pyrMeanShiftFiltering(segmented_img, Blur_img, 20, 45, 3);
        cvtColor(Blur_img, greyimg, cv::COLOR_HSV2BGR);
        cvtColor(greyimg, greyimg, cv::COLOR_BGR2GRAY);

        SLIC slic;
        int numSuperpixel = 3;
        slic.GenerateSuperpixels(greyimg, numSuperpixel);
        if (img.channels() == 3)
        {
            result = slic.GetImgWithContours(cv::Scalar(230, 230, 250));
        }
        else
        {
            result = slic.GetImgWithContours(cv::Scalar(255));
        }

        int *labels;

        labels = slic.GetLabel();
        int x = 10;
        int y = 20;

        for (int i = 0; i < result.rows; i++)
        { // makes the cluster 2 integers 1 , and others zero
            for (int i2 = 0; i2 < result.cols; i2++)
            {
                if (1 == labels[i * result.cols + i2])
                {
                    labels[i * result.cols + i2] = 1;
                    continue;
                }
                labels[i * result.cols + i2] = 0;
            }
        }

        for (int i = 0; i < img.rows; i++)
        { // makes the cluster 2 integers 1 , and others zero
            for (int i2 = 0; i2 < img.cols; i2++)
            {
                if (1 == labels[i * result.cols + i2]) //|| 0 == labels[i * result.cols + i2]
                {
                    if ((resultHSV.at<cv::Vec3b>(i, i2)[2] > 30 && resultHSV.at<cv::Vec3b>(i, i2)[2] < 240) && (resultHSV.at<cv::Vec3b>(i, i2)[2] > 30 && resultHSV.at<cv::Vec3b>(i, i2)[2] < 240))
                    {
                        // platform_log("Here");
                        img.at<cv::Vec3b>(i, i2)[0] = 18;
                        img.at<cv::Vec3b>(i, i2)[1] = 80;
                        img.at<cv::Vec3b>(i, i2)[2] = img.at<cv::Vec3b>(i, i2)[2] + 20;
                        continue;
                    }
                }
            }
        }

        // cvtColor(result, result, cv::COLOR_GRAY2BGR);
        // cvtColor(Blur_img, greyimg, cv::COLOR_BGR2HSV);

        // // PIGMENTATION
        // vector<Mat> hsv_vec;
        // split(resultHSV, hsv_vec); // this is an opencv function
        // cv::Mat &H = hsv_vec[0];
        // cv::Mat &S = hsv_vec[1];
        // cv::Mat &V = hsv_vec[2];
        // // resultHSV = (V > 65); // non-zero pixels in the original image
        // H = 30; // H is between 0-180 in OpenCV
        // S = 178;
        // // V = V.mul((V + 20)) / (V + 1);
        // // Mat V_temp = V.mul(V + 20);
        // V = V + 20;
        // // cv::divide(V_temp, V_temp + 1, V);
        // merge(hsv_vec, resultHSV);
        // HSVImage = resultHSV; // pigmented image

        // // OVERLAY
        // cv::Mat base = cv::imread(inputImagePath);
        // cv::cvtColor(segmented_img, segmented_img, cv::COLOR_HSV2BGR);
        // subtract(base, segmented_img, base);
        // // cvtColor(base, base, cv::COLOR_BGR2HSV);
        // cv::Mat finalImage;
        // cvtColor(HSVImage, finalImage, cv::COLOR_HSV2BGR); // or rgb

        // // add(finalImage, base, base);
        // base = finalImage + base;

        // OUTPUTS
        cvtColor(img, img, cv::COLOR_HSV2BGR); // or rgb
        // cvtColor(segmented_img, segmented_img, cv::COLOR_HSV2BGR); // or rgb

        platform_log("Output Path: %s", outputPath);
        imwrite(outputPath, img); // then compare withy img
        platform_log("Image writed again ");
    }
}
