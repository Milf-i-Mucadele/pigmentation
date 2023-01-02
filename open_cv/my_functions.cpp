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
        cv::Mat img0 = cv::imread(inputImagePath), imgGray, result, result_2, final_image;

        cv::Mat markerMask;

        cvtColor(img0, markerMask, COLOR_BGR2GRAY);
        markerMask = Scalar::all(0);
        ifstream MyReadFile(linesFromUser);
        platform_log("1111");
        platform_log(linesFromUser);
        platform_log("1111");
        std::string line;
        while (std::getline(MyReadFile, line)) {
            platform_log("1");
            std::vector<int> integers;

            // Declare an input string stream and pass the line to it
            std::istringstream iss(line);
            platform_log("2");

            // Split the line by a space
            std::string token;
            while (iss >> token) {
            // Convert the token to an integer
            int n = std::stoi(token);
            // Add the integer to the vector
            integers.push_back(n);
            }
            platform_log("3");
            Point p1(integers[0], integers[1]);
            Point p2(integers[2], integers[3]);
            platform_log("4"); 
            cv::line(markerMask, p1, p2, Scalar::all(255), 5, 8, 0);
            platform_log("Output Path: %d", integers[3]);
            }
            //platform_log("Output Path: %d", 1);
            int i, j, compCount = 0;
            vector<vector<Point> > contours;
            vector<Vec4i> hierarchy;
            findContours(markerMask, contours, hierarchy, RETR_CCOMP, CHAIN_APPROX_SIMPLE);
            platform_log("5"); 

            Mat markers(markerMask.size(), CV_32S);
            markers = Scalar::all(0);
            int idx = 0;
            for( ; idx >= 0; idx = hierarchy[idx][0], compCount++ )
                drawContours(markers, contours, idx, Scalar::all(compCount+1), -1, 8, hierarchy, INT_MAX);
            platform_log("6"); 

            vector<Vec3b> colorTab;
            for( i = 1; i < compCount; i++ )
            {   
                int b = i*10;
                int g = i*10;
                int r = i*10;
                //int b = theRNG().uniform(0, 255);
                //int g = theRNG().uniform(0, 255);
                //int r = theRNG().uniform(0, 255);
                colorTab.push_back(Vec3b(b,g,r));
            }
            watershed( img0, markers );
            platform_log("7"); 

            Mat wshed(markers.size(), CV_8UC3);
            // paint the watershed image
            /*
            for( i = 0; i < markers.rows; i++ )
                for( j = 0; j < markers.cols; j++ )
                {
                    int index = markers.at<int>(i,j);
                    //cout << index;
                    if( index == -1 )
                        wshed.at<Vec3b>(i,j) = Vec3b(255,255,255);
                    else if( index <= 0 || index > compCount )
                        wshed.at<Vec3b>(i,j) = Vec3b(0,0,0);
                    else
                        wshed.at<Vec3b>(i,j) = colorTab[index - 1];
                }
            */
            platform_log("8"); 
            //wshed = wshed*0.5 + imgGray*0.5;
            //imshow( "watershed 1", wshed  );
            cvtColor(wshed, wshed, COLOR_BGR2GRAY);
            threshold(wshed, wshed, 0, compCount*10-5, THRESH_BINARY);
            bitwise_and(img0, img0, result, wshed);

            cvtColor(result, result, COLOR_BGR2HSV);
            
            vector<Mat> hsv_vec;
            split(result, hsv_vec); //this is an opencv function

            cv::Mat& h = hsv_vec[0];
            cv::Mat& s = hsv_vec[1];
            cv::Mat& v = hsv_vec[2];
            h.setTo(0, v > 1);
            s.setTo(100, v > 1);
            merge(hsv_vec, result);
            cvtColor(result, result, COLOR_HSV2BGR);
            //imshow( "watershed 1", wshed);

            threshold(wshed, wshed, 0, 255, THRESH_BINARY_INV);
            bitwise_and(img0, img0, result_2, wshed);

            final_image = result + result_2;
            
        imwrite(outputPath, final_image);
    }
}
