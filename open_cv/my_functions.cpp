#include <opencv2/opencv.hpp>
#include <chrono>

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
        platform_log("PATH %s: ", inputImagePath);
        cv::Mat img = cv::imread(inputImagePath);
        platform_log("Length: %d", img.rows);

        // BGR -> HSV changing part
        cv::Mat HSVImage;
        cvtColor(img, HSVImage, cv::COLOR_BGR2HSV);

        // changing Hue value
        cv::Mat hsv = HSVImage.clone();
        vector<Mat> hsv_vec;
        split(hsv, hsv_vec); // this is an opencv function
        cv::Mat &H = hsv_vec[0];
        cv::Mat &S = hsv_vec[1];
        cv::Mat &V = hsv_vec[2];
        // S = 0;
        // hsv = (V > 10); // non-zero pixels in the original image

        H = 23; // H is between 0-180 in OpenCV
        merge(hsv_vec, hsv);
        HSVImage = hsv; // according to your code

        // showing HSV image Notice: imshow always renders in BGR space
        cv::Mat finalImage;
        cvtColor(HSVImage, finalImage, cv::COLOR_HSV2BGR);
        platform_log("Output Path: %s", outputPath);
        imwrite(outputPath, finalImage);
        platform_log("Image writed again ");
    }
}
