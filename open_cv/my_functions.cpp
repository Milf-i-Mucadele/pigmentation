#include <opencv2/opencv.hpp>
#include <chrono>
#include "slic.h"
#include <fstream>

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
        //platform_log("PATH %s: ", inputImagePath);
        cv::Mat img = cv::imread(inputImagePath);
        //platform_log("Length: %d", img.rows);
        //cvtColor(img, img, COLOR_BGR2RGB);
        cv::Mat hsv, mask, res, result;

        //medianBlur(img,blur, 5);
        //bilateralFilter(blur2,blur3, 9, 75, 75);
        cvtColor(img, hsv, COLOR_BGR2HSV);
        GaussianBlur(hsv,hsv, Size(5, 5), 0);
        /*
        vector<Mat> hsv_vec;
        split(hsv, hsv_vec); //this is an opencv function

        cv::Mat& h = hsv_vec[0];
        cv::Mat& s = hsv_vec[1];
        cv::Mat& l = hsv_vec[2];
        
        //l.setTo(130, l > 30);
        merge(hsv_vec, hsv); 
        */
        cv::Scalar minHSV = cv::Scalar(0, 130, 30);
        cv::Scalar maxHSV = cv::Scalar(20, 255, 240); 

        inRange(hsv, minHSV, maxHSV, mask);
        bitwise_and(hsv, hsv, res, mask);
        cvtColor(res, res, COLOR_HSV2BGR);

        
        vector<Mat> hsv_vec;
        split(res, hsv_vec); //this is an opencv function

        cv::Mat& h = hsv_vec[0];
        cv::Mat& s = hsv_vec[1];
        cv::Mat& v = hsv_vec[2];
        h.setTo(100, v > 1);
        s.setTo(100, v > 1);
        merge(hsv_vec, res); 
        

        threshold(mask, mask, 0, 255, THRESH_BINARY_INV);
        bitwise_and(img, img, result, mask);
        img = result + res;

        /*
        SLIC slic;
    	int numSuperpixel = 5;
    	slic.GenerateSuperpixels(res, numSuperpixel);

        if (img.channels() == 3) 
            result = slic.GetImgWithContours(cv::Scalar(10, 0, 255));
        else
            result = slic.GetImgWithContours(cv::Scalar(128));
        //*/
  
        /*
        cvtColor(res, res, COLOR_RGB2GRAY);
        vector<vector<Point> > contours;
        vector<Vec4i> hierarchy;
        Canny( res, canny_output, 100, 100*2 );

        findContours( canny_output, contours, hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE );

        int morph_size = 5;
        Mat element = getStructuringElement(MORPH_RECT, Size(2 * morph_size + 1,2 * morph_size + 1), Point(morph_size, morph_size));
        
        dilate(canny_output, canny_output, element, Point(-1, -1), 1);

        std::sort(contours.begin(), contours.end(), [](const vector<Point>& c1, const vector<Point>& c2){return contourArea(c1, false) < contourArea(c2, false);});

        Mat drawing = Mat::zeros( canny_output.size(), CV_8UC3 );
        drawContours( drawing, contours, -1, Scalar(0, 255, 0), 10, LINE_8, hierarchy, 0 );
        */
       
        //cvtColor(HSVImage, finalImage, cv::COLOR_HSV2BGR);
        //platform_log("Output Path: %s", outputPath);
        imwrite(outputPath, img);
        //platform_log("Image writed again ");
    }



     __attribute__((visibility("default"))) __attribute__((used)) void water_shed(char *inputImagePath, char *outputPath, char *linesFromUser)
    {
        cv::Mat img = cv::imread(inputImagePath);
        cv::Mat markerMask;

        cvtColor(img, markerMask, COLOR_BGR2GRAY);
        markerMask = Scalar::all(0);
        ifstream MyReadFile(linesFromUser);

        std::string line;
        while (std::getline(MyReadFile, line)) {
            std::vector<int> integers;

            // Declare an input string stream and pass the line to it
            std::istringstream iss(line);

            // Split the line by a space
            std::string token;
            while (iss >> token) {
            // Convert the token to an integer
            int n = std::stoi(token);
            // Add the integer to the vector
            integers.push_back(n);
            }
            platform_log("Output Path: %d", integers.size());
            Point p1(integers[0], integers[1]);
            Point p2(integers[2], integers[3]);
            cv::line(markerMask, p1, p2, Scalar::all(255), 5, 8, 0);   
            }
        platform_log("Output Path: %d", 1);

        imwrite(outputPath, markerMask);
    }
}
