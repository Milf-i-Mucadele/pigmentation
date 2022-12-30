#include <opencv2/opencv.hpp>
#include <chrono>
#include "slic.h"
#include <opencv2/imgproc.hpp>

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
        cv::Scalar minHSV = cv::Scalar(0, 130, 30);
        cv::Scalar maxHSV = cv::Scalar(20, 255, 240);
        cv::Mat maskHSV, resultHSV;
        cv::inRange(hsv, minHSV, maxHSV, maskHSV);
        // bitwise_not(maskHSV, maskHSV);
        cv::bitwise_and(hsv, hsv, resultHSV, maskHSV);
        cv::Mat segmented_img;
        segmented_img = resultHSV.clone();

        // PIGMENTATION
        vector<Mat> hsv_vec;
        split(resultHSV, hsv_vec); // this is an opencv function
        cv::Mat &H = hsv_vec[0];
        cv::Mat &S = hsv_vec[1];
        cv::Mat &V = hsv_vec[2];
        // resultHSV = (V > 65); // non-zero pixels in the original image
        H = 25; // H is between 0-180 in OpenCV
        S = 255;
        merge(hsv_vec, resultHSV);
        HSVImage = resultHSV; // pigmented image

        // OVERLAY
        cv::Mat base = cv::imread(inputImagePath);
        subtract(base, segmented_img, base);
        // cvtColor(base, base, cv::COLOR_BGR2HSV);
        cv::Mat finalImage;
        cvtColor(HSVImage, finalImage, cv::COLOR_HSV2BGR); // or rgb

        add(finalImage, base, base);

        // OUTPUTS
        platform_log("Output Path: %s", outputPath);
        imwrite(outputPath, base);
        platform_log("Image writed again ");
    }
}
