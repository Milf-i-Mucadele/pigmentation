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



     __attribute__((visibility("default"))) __attribute__((used)) void water_shed(char *inputImagePath, char *outputPath, char *inputXorImagePath)
    {
        platform_log("***WATERSHED***");
        cv::Mat img0 = cv::imread(inputImagePath),xor_image,result,result_2,final_image;
        cv::Mat img_xor = cv::imread(inputXorImagePath);
        platform_log("%d,%d",img0.rows, img0.cols);
        platform_log("%d,%d",img_xor.rows, img_xor.cols);

        platform_log(inputImagePath);
        platform_log(outputPath);
        platform_log(inputXorImagePath);

        bitwise_xor(img0, img_xor, xor_image);
        cvtColor(xor_image, xor_image, COLOR_BGR2GRAY);
        threshold(xor_image, xor_image, 0, 255, THRESH_BINARY);
        cv::Mat element_ = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(5, 5));
        //erode( wshed, wshed, element );
        //imshow( "erode", wshed );
        dilate( xor_image, xor_image, element_ );
        erode( xor_image, xor_image, element_ );
        erode( xor_image, xor_image, element_ );
        erode( xor_image, xor_image, element_ );
        erode( xor_image, xor_image, element_ );
        //imwrite(outputPath, xor_image);
        //cout << xor_image;
        int i, j,mini=999,minj=999,maxi=-10,maxj=-10, compCount = 0;
        //imshow("1",xor_image);
        for( i = 0; i < xor_image.rows; i++ )
            for( j = 0; j < xor_image.cols; j++ )
            {   
                //cout << xor_image.at<uchar>(i,j);
                if( xor_image.at<uchar>(i,j) == 255 ){
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
        platform_log("%d,%d,%d,%d",mini,minj,maxi,maxj);
        // Top Left Corner
        Point p1(minj-50, mini-50);
    
        // Bottom Right Corner
        Point p2(maxj+50, maxi+50);
        //Point center(minj, mini);//Declaring the center point
        //circle(xor_image, center,10, Scalar(0,255,0), 3);//Using circle()function to draw the line//
        //Point center2(maxj, maxi);//Declaring the center point
        //circle(xor_image, center2,10, Scalar(0,255,0), 3);//Using circle()function to draw the line//
        rectangle(xor_image, p1,p2, Scalar(255,255,255), 2);

        //cout << mini << endl;
        //cout << maxi << endl;
        //cout << minj << endl;
        //cout << maxj << endl;
        cvtColor(xor_image, xor_image, COLOR_BGR2GRAY);
        //imshow("2",xor_image);
        vector<vector<Point> > contours;
        vector<Vec4i> hierarchy;
        findContours(xor_image, contours, hierarchy, RETR_CCOMP, CHAIN_APPROX_SIMPLE);

        Mat markers(xor_image.size(), CV_32S);
        markers = Scalar::all(0);
        int idx = 0;
        for( ; idx >= 0; idx = hierarchy[idx][0], compCount++ )
            drawContours(markers, contours, idx, Scalar::all(compCount+1), -1, 8, hierarchy, INT_MAX);

        vector<Vec3b> colorTab;
        for( i = 0; i < compCount; i++ )
        {
            int b = i*20;
            int g = i*20;
            int r = i*20;
            colorTab.push_back(Vec3b(b,g,r));
        }
        watershed( img0, markers );

        Mat wshed(markers.size(), CV_8UC3);
        // paint the watershed image
        for( i = 0; i < markers.rows; i++ )
            for( j = 0; j < markers.cols; j++ )
            {
                int index = markers.at<int>(i,j);
                //platform_log("%d",index);
                if( index == -1 )
                    wshed.at<cv::Vec3b>(i,j) = cv::Vec3b(255,255,255);
                else if( index <= 0 || index > compCount )
                    wshed.at<cv::Vec3b>(i,j) = cv::Vec3b(0,0,0);
                else
                    wshed.at<cv::Vec3b>(i,j) = colorTab[index - 1];
            }
        //wshed = wshed*0.5 + imgGray*0.5;
        //imshow( "watershed1", wshed );
        cvtColor(wshed, wshed, COLOR_BGR2GRAY);
        //cout << compCount;
        threshold(wshed, wshed, (compCount-2)*20, 255, THRESH_BINARY_INV);
        //bitwise_and(img0, img0, result, wshed);
        //imshow( "watershed2", wshed );
        cv::Mat element = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(5, 5));
        //erode( wshed, wshed, element );
        //imshow( "erode", wshed );
        dilate( wshed, wshed, element );
        //imshow( "dilate", wshed );
        bitwise_and(img0, img0, result, wshed);
        //cout << wshed;
        //threshold(wshed, wshed, 0, 255, THRESH_BINARY);

        cvtColor(result, result, COLOR_BGR2HSV);
        
        vector<Mat> hsv_vec;
        split(result, hsv_vec); //this is an opencv function

        cv::Mat& h = hsv_vec[0];
        cv::Mat& s = hsv_vec[1];
        cv::Mat& v = hsv_vec[2];
        h.setTo(50, v > 1);
        s.setTo(100, v > 1);
        merge(hsv_vec, result);
        cvtColor(result, result, COLOR_HSV2BGR);
        //imshow( "result", result );
        threshold(wshed, wshed, 0, 255, THRESH_BINARY_INV);
        //imshow( "watershed 1", wshed);
        bitwise_and(img0, img0, result_2, wshed);

        final_image = result + result_2;            
        imwrite(outputPath, final_image);
    }
}
