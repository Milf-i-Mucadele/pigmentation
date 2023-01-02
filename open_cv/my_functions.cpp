#include <opencv2/opencv.hpp>
#include <chrono>
#include "slic.h"
#include <opencv2/imgproc.hpp>
#include <iostream>
#include <fstream>
#include <string>
#include <opencv2/imgcodecs.hpp>

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

    __attribute__((visibility("default"))) __attribute__((used)) void convertImageToGrayImage(char *inputImagePath, char *outputPath)
    {
        // inputimage path,color user wants, algortihm, reference points/tap point, output path

        // TXT FILE  READING
        string myText;
        ifstream MyReadFile("open_cv/loo.txt");
        while (getline(MyReadFile, myText))
        {
            ofstream MyFile("open_cv/output.txt");
            // Write to the file
            MyFile << "Files can be tricky, but it is fun enough!";

            // Close the file
            MyFile.close();
            platform_log("logged");
            platform_log("%s", myText.c_str());
        }

        // Close the file
        MyReadFile.close();

        platform_log("PATH %s: ", inputImagePath);
        cv::Mat img = cv::imread(inputImagePath);
        platform_log("Length: %d", img.rows);

        // BGR -> HSV changing part
        cv::Mat HSVImage;
        cv::Mat original = HSVImage.clone();
        cv::Mat clone;
        clone = HSVImage.clone();
        cvtColor(img, HSVImage, cv::COLOR_BGR2HSV);

        // changing Hue value
        cv::Mat hsv = HSVImage.clone();
        // here
        // cv::Scalar minHSV = cv::Scalar(0, 0, 0);//wc door
        cv::Scalar minHSV = cv::Scalar(4, 135, 0); // bed

        cv::Scalar maxHSV = cv::Scalar(20, 255, 240); // bed

        // cv::Scalar maxHSV = cv::Scalar(30, 255, 240); //wc door
        cv::Mat maskHSV, resultHSV;
        cv::inRange(hsv, minHSV, maxHSV, maskHSV);
        // bitwise_not(maskHSV, maskHSV);
        cv::bitwise_and(hsv, hsv, resultHSV, maskHSV);
        cv::Mat segmented_img;
        segmented_img = resultHSV.clone();

        // SLIC Superpixels
        Mat result;
        Mat Blur_img;
        Mat greyimg;

        pyrMeanShiftFiltering(segmented_img, Blur_img, 20, 45, 3);
        // GaussianBlur(segmented_img, Blur_img, Size(7, 7), 5, 0); // Now finally adding blur to the image
        cvtColor(Blur_img, greyimg, cv::COLOR_HSV2BGR);
        cvtColor(greyimg, greyimg, cv::COLOR_BGR2GRAY);

        SLIC slic;
        int numSuperpixel = 4;
        slic.GenerateSuperpixels(greyimg, numSuperpixel);
        if (img.channels() == 3)
        {
            result = slic.GetImgWithContours(cv::Scalar(230, 230, 250));
        }
        else
        {
            result = slic.GetImgWithContours(cv::Scalar(255));
        }
        cvtColor(result, result, cv::COLOR_GRAY2BGR);
        cvtColor(Blur_img, greyimg, cv::COLOR_BGR2HSV);

        // PIGMENTATION
        vector<Mat> hsv_vec;
        split(resultHSV, hsv_vec); // this is an opencv function
        cv::Mat &H = hsv_vec[0];
        cv::Mat &S = hsv_vec[1];
        cv::Mat &V = hsv_vec[2];
        // resultHSV = (V > 65); // non-zero pixels in the original image
        H = 30; // H is between 0-180 in OpenCV
        S = 178;
        // V = V.mul((V + 20)) / (V + 1);
        // Mat V_temp = V.mul(V + 20);
        V = V + 20;
        // cv::divide(V_temp, V_temp + 1, V);
        merge(hsv_vec, resultHSV);
        HSVImage = resultHSV; // pigmented image

        // OVERLAY
        cv::Mat base = cv::imread(inputImagePath);
        cv::cvtColor(segmented_img, segmented_img, cv::COLOR_HSV2BGR);
        subtract(base, segmented_img, base);
        // cvtColor(base, base, cv::COLOR_BGR2HSV);
        cv::Mat finalImage;
        cvtColor(HSVImage, finalImage, cv::COLOR_HSV2BGR); // or rgb

        // add(finalImage, base, base);
        base = finalImage + base;

        // OUTPUTS
        platform_log("Output Path: %s", outputPath);
        imwrite(outputPath, base); // then compare withy base
        platform_log("Image writed again ");
    }
}
