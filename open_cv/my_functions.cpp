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
#include <cstdio>
#include <cstdlib>

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

    __attribute__((visibility("default"))) __attribute__((used)) void convertImageToGrayImage(char *inputImagePath, char *outputPath, char *tappoint, char *colorhex)

    {
        try {
    platform_log("PATH %s: ", inputImagePath);
        platform_log("Tappoint %s", tappoint);
        platform_log("colorhex %s", colorhex);
        vector<std::pair<int, int>> coordinates = parse_coordinates(tappoint);

        platform_log("coordinates: %d , %d", coordinates[0].first, coordinates[0].second);
        Point p1(coordinates[0].first, coordinates[0].second); // todo: this will be read fromn txt file 350,520 for kapi change here

        int r, g, b;
        std::sscanf(colorhex, "#%02x%02x%02x", &r, &g, &b);

        platform_log("r,g,b: %d,%d,%d", r, g, b);

        cv::Mat src = cv::Mat(1, 1, CV_8UC3, cv::Scalar(r, g, b));
        cv::Mat desired;

        cv::cvtColor(src, desired, cv::COLOR_RGB2HSV);
        int desired_h = desired.at<cv::Vec3b>(0, 0)[0];
        int desired_sat = desired.at<cv::Vec3b>(0, 0)[1];
        int desired_value = desired.at<cv::Vec3b>(0, 0)[2];
        platform_log("the desried H S V color : %d,%d,%d", desired_h, desired_sat, desired_value);

        cv::Mat img = cv::imread(inputImagePath);
        platform_log("Length row: %i", img.rows);
        platform_log("Length column: %i", img.cols);

        // BGR -> HSV changing part
        cv::Mat HSVImage;
        cvtColor(img, HSVImage, cv::COLOR_BGR2HSV);
        cvtColor(img, img, cv::COLOR_BGR2HSV);
        // BLUR THE SEGMENTED IMAGE  todo maybe this place will come after segmentation
        blur(HSVImage, HSVImage, Size(30, 30));

        // COLOR SEGMENTATION
        cv::Mat hsv = HSVImage.clone();

        cv::Vec3b pixel = hsv.at<cv::Vec3b>(coordinates[0].second, coordinates[0].first);
        int hue = pixel[0];
        int min_sat = pixel[1];
        int value = pixel[2];

        int high_hue = std::max((hue-20) % 180, (hue+20)% 180);
        int low_hue = std::min((hue-20) % 180, (hue-20) % 180);
        if (high_hue == low_hue)
        {
            high_hue = 180;
        }

        platform_log("tappoint hue, sat : %d,%d", hue, min_sat);

        int black ; 
        cv::Scalar minHSV, maxHSV;

        if(value <40) {  //black image

        black =1;

        minHSV = cv::Scalar(0, min_sat - 60, 0); // bed
        maxHSV = cv::Scalar(180, 255, 40);         // bed was +30

        }

        else if (min_sat <40) {
        black =0;
        minHSV = cv::Scalar(0, min_sat - 60, 0); // bed
        maxHSV = cv::Scalar(180, 255, 255);         // bed was +30
        }
        else {
        black =0;
        minHSV = cv::Scalar(low_hue, min_sat - 60, 20); // bed
        maxHSV = cv::Scalar(high_hue, 255, 240);         // bed was +30
        }

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
        //cvtColor(result, result, COLOR_BGR2HSV);

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

                int counter = 0;
                int value_mean = 0;
                int sat_mean = 0;
                for(int i=0; i<h.rows; i++){
                    for(int j=0; j<h.cols; j++){
                        uchar &value = v.at<uchar>(i, j);
                        uchar &sat = s.at<uchar>(i, j);
                        if (value > 0){
                        value_mean += value; 
                        sat_mean += sat; 

                        counter += 1;
   
                        }
                    }
                }


                value_mean = value_mean / counter;
                sat_mean = sat_mean / counter;

                //cout << value_mean << endl;
                //cout << sat_mean << endl;
                
                h.setTo(desired_h, v > 0);

                if (desired_value >= 80 && desired_sat >= 80) {
                    //cout << "normal wanted" << endl;

                if (sat_mean < 80 && value_mean >= 80){
                    //cout << "white image" << endl;
                    for(int i=0; i<h.rows; i++){
                        for(int j=0; j<h.cols; j++){
                            uchar &value = v.at<uchar>(i, j);
                            uchar &sat = s.at<uchar>(i, j);
                            if (value > 0){
                                sat = max(10, min(sat + desired_sat - sat_mean, (int)255));
                                value = max(10, min(value + desired_value - value_mean, (int)255));

                                }
                            }
                            
                            }
                        }
                    
                else if (value_mean < 80 && sat_mean >= 80){
                    //cout << "black image" << endl;
                    s.setTo(desired_sat, v > 0);
                    for(int i=0; i<h.rows; i++){
                        for(int j=0; j<h.cols; j++){
                            uchar &value = v.at<uchar>(i, j);
                            if (value > 0){
                            value = max(10, min(value + desired_value - value_mean, (int)255));

                                }
                            }
                            
                            }
                        }
            
                else if (value_mean < 80 && sat_mean < 80){
                    //cout << "ultra dark image" << endl;
                    for(int i=0; i<h.rows; i++){
                        for(int j=0; j<h.cols; j++){
                            uchar &value = v.at<uchar>(i, j);
                            uchar &sat = s.at<uchar>(i, j);
                            if (value > 0){
                            value = max(10, min(value + desired_value - value_mean, (int)255));
                            sat = max(10, min(sat + desired_sat - sat_mean, (int)255));
                                }
                            }
                        }
                }

                else {
                    //cout << "normal image" << endl;
                    s.setTo(desired_sat, v > 0);

                    for(int i=0; i<h.rows; i++){
                        for(int j=0; j<h.cols; j++){
                            uchar &value = v.at<uchar>(i, j);
                            if (value > 0){
                            value = max(10, min(value + desired_value - value_mean, (int)255));
                                }
                            }
                        }
                }
                }


            else if (desired_value >= 80 && desired_sat < 80) {
                //cout << "white wanted" << endl;

                
                  if (sat_mean < 80 && value_mean >= 80 ){
                    //cout << "white image" << endl;
                    for(int i=0; i<h.rows; i++){
                        for(int j=0; j<h.cols; j++){
                            uchar &value = v.at<uchar>(i, j);
                            uchar &sat = s.at<uchar>(i, j);
                            if (value > 0){
                                sat = max(10, min(sat + desired_sat - sat_mean, (int)255));
                                value = max(10 ,min(desired_value, (int)255));
                                }
                            }
                            }
                        }
                else{
                    
                for(int i=0; i<h.rows; i++){
                    for(int j=0; j<h.cols; j++){
                        uchar &value = v.at<uchar>(i, j);
                        uchar &sat = s.at<uchar>(i, j);
                        if (value > 0){
                            //sat = max(1, min((sat * 40)/255 + desired_sat-20, (int)255));
                            sat = max(10, min(sat + desired_sat - sat_mean, (int)255));

                            value = max(10 ,min(desired_value, (int)255));
                            }
                        }
                    }
                }
                }

            else if (desired_value < 80 && desired_sat >= 80) {
                //cout << "black wanted" << endl;
                    
                  if (sat_mean < 80){
                    //cout << "white image" << endl;
                    for(int i=0; i<h.rows; i++){
                        for(int j=0; j<h.cols; j++){
                            uchar &value = v.at<uchar>(i, j);
                            uchar &sat = s.at<uchar>(i, j);
                            if (value > 0){
                                sat = max(10,min(sat + desired_sat /2, (int)255));
                            value = max(10, min(value + desired_value - value_mean, (int)255));
                                }
                            }
                            
                            }
                        }
                else{
                        
                for(int i=0; i<h.rows; i++){
                    for(int j=0; j<h.cols; j++){
                        uchar &value = v.at<uchar>(i, j);
                        if (value > 0){
                            value = max(10, min(value + desired_value - value_mean, (int)255));
                            //cout << desired_value - value_mean;
                            }
                        }
                    }
                }
                }

            else if (desired_value < 80 && desired_sat < 80) {
                //cout << "ultra black wanted" << endl;

                for(int i=0; i<h.rows; i++){
                    for(int j=0; j<h.cols; j++){
                        uchar &value = v.at<uchar>(i, j);
                        uchar &sat = s.at<uchar>(i, j);
                        if (value > 0){
                            value = max(10, min(value + desired_value - value_mean, (int)255));
                            }
                        }
                    }
                }
        merge(hsv_vec, result);

        // BACKGROUND
        cvtColor(result, result, COLOR_HSV2BGR);
        cvtColor(result, result2, COLOR_BGR2GRAY);
        //cvtColor(img, img, cv::COLOR_BGR2HSV);

        threshold(result2, result2, 0, 255, THRESH_BINARY_INV);
        bitwise_and(img, img, background, result2);
        cvtColor(background, background, cv::COLOR_HSV2BGR); // or rgb

        final_image = result + background;

        // RETURNING
        platform_log("Output Path: %s", outputPath);
        imwrite(outputPath, final_image); // then compare withy img
        platform_log("Image writed again ");

          }
    catch (const std::exception& e) {
        cv::Mat img0 = cv::imread(inputImagePath);
        imwrite(outputPath, img0);
  }

    }
    

    __attribute__((visibility("default"))) __attribute__((used)) void water_shed(char *inputImagePath, char *outputPath, char *inputXorImagePath,  char *colorhex)
    {
          try {
        platform_log("***WATERSHED***");
        cv::Mat img0 = cv::imread(inputImagePath), imgGray, final_image, xor_image, xor_image_copy, temp, temp2, result, result_2, result_grey;
        cv::Mat img_xor = cv::imread(inputXorImagePath);

        int r, g, b;
        std::sscanf(colorhex, "#%02x%02x%02x", &r, &g, &b);

        platform_log("r,g,b: %d,%d,%d", r, g, b);

        cv::Mat src = cv::Mat(1, 1, CV_8UC3, cv::Scalar(r, g, b));
        cv::Mat desired;
        cv::cvtColor(src, desired, cv::COLOR_RGB2HSV);
        int desired_h = desired.at<cv::Vec3b>(0, 0)[0];
        int desired_sat = desired.at<cv::Vec3b>(0, 0)[1];
        int desired_value = desired.at<cv::Vec3b>(0, 0)[2];

        platform_log("%d,%d", img0.rows, img0.cols);
        platform_log("%d,%d", img_xor.rows, img_xor.cols);

        platform_log(inputImagePath);
        platform_log(outputPath);
        platform_log(inputXorImagePath);

        int offset = 50;
        int i, j, compCount = 0;
        vector<vector<Point>> contours_xor, contours_rect, contours;
        vector<Vec4i> hierarchy;

        bitwise_xor(img0, img_xor, xor_image);
        cvtColor(xor_image, xor_image, COLOR_BGR2GRAY);
        threshold(xor_image, xor_image, 0, 255, THRESH_BINARY);
        cv::Mat element_ = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(5, 5));
        erode(xor_image, xor_image, element_);
        erode(xor_image, xor_image, element_);
        // erode( xor_image, xor_image, element_ );
        // erode( xor_image, xor_image, element_ );

        xor_image.copyTo(xor_image_copy);

        findContours(xor_image_copy, contours_xor, RETR_TREE, CHAIN_APPROX_SIMPLE);
        cvtColor(xor_image_copy, xor_image_copy, COLOR_GRAY2BGR);

        for (auto cnt : contours_xor)
        {
            cv::Rect rect = cv::boundingRect(cnt);
            rect.width = (int)(rect.width + offset * 2);
            rect.height = (int)(rect.height + offset * 2);
            rect.x = (int)(rect.x - offset);
            rect.y = (int)(rect.y - offset);
            cv::rectangle(xor_image_copy, rect, cv::Scalar(255, 255, 255), -1);
        }

        cvtColor(xor_image_copy, xor_image_copy, COLOR_BGR2GRAY);
        findContours(xor_image_copy, contours_rect, RETR_TREE, CHAIN_APPROX_SIMPLE);

            for (auto cnt : contours_rect) {
                Mat result, result_2;
                cv::Rect rect = cv::boundingRect(cnt);

                Mat temp(xor_image_copy.size(), CV_8UC3,(0,0,0));
                Mat temp2(xor_image_copy.size(), CV_8UC3,(0,0,0));
                cv::rectangle(temp, rect, cv::Scalar(255, 255, 255), -1);

                cvtColor(temp, temp, COLOR_BGR2GRAY);
                bitwise_and(xor_image, xor_image, temp2, temp);
                cv::rectangle(temp2, rect, cv::Scalar(255, 255, 255), 3);

                findContours(temp2, contours, hierarchy, RETR_CCOMP, CHAIN_APPROX_SIMPLE);
                if( contours.empty() )
                    continue;
                Mat markers(xor_image.size(), CV_32S);
                markers = Scalar::all(0);
                int idx = 0;
                for( ; idx >= 0; idx = hierarchy[idx][0], compCount++ )
                    drawContours(markers, contours, idx, Scalar::all(compCount+1), -1, 8, hierarchy, INT_MAX);
                if( compCount == 0 )
                    continue;
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
                for( i = 0; i < markers.rows; i++ )
                    for( j = 0; j < markers.cols; j++ )
                    {
                        int index = markers.at<int>(i,j);
                        if( index == -1 )
                            wshed.at<Vec3b>(i,j) = Vec3b(255,255,255);
                        else if( index <= 0 || index > compCount )
                            wshed.at<Vec3b>(i,j) = Vec3b(0,0,0);
                        else
                            wshed.at<Vec3b>(i,j) = colorTab[index - 1];
                    }

                cvtColor(wshed, wshed, COLOR_BGR2GRAY);
                threshold(wshed, wshed, (compCount-2)*20, 255, THRESH_BINARY_INV);

                cv::Mat element = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(5, 5));
                dilate( wshed, wshed, element );
                bitwise_and(img0, img0, result, wshed);

                cvtColor(result, result, COLOR_BGR2HSV);
                vector<Mat> hsv_vec;
                split(result, hsv_vec);

                cv::Mat& h = hsv_vec[0];
                cv::Mat& s = hsv_vec[1];
                cv::Mat& v = hsv_vec[2];
                
                int counter = 0;
                int value_mean = 0;
                int sat_mean = 0;
                for(int i=0; i<h.rows; i++){
                    for(int j=0; j<h.cols; j++){
                        uchar &value = v.at<uchar>(i, j);
                        uchar &sat = s.at<uchar>(i, j);
                        if (value > 1){
                        value_mean += value; 
                        sat_mean += sat; 

                        counter += 1;
   
                        }
                    }
                }

                value_mean = value_mean / counter;
                sat_mean = sat_mean / counter;
                
                h.setTo(desired_h, v > 0);

                if (desired_value >= 80 && desired_sat >= 80) {

                if (sat_mean < 80 && value_mean >= 80){
                    for(int i=0; i<h.rows; i++){
                        for(int j=0; j<h.cols; j++){
                            uchar &value = v.at<uchar>(i, j);
                            uchar &sat = s.at<uchar>(i, j);
                            if (value > 0){
                                sat = max(10, min(sat + desired_sat - sat_mean, (int)255));
                                value = max(10, min(value + desired_value - value_mean, (int)255));

                                }
                            }
                            
                            }
                        }
                    
                else if (value_mean < 80 && sat_mean >= 80){
                    s.setTo(desired_sat, v > 0);
                    for(int i=0; i<h.rows; i++){
                        for(int j=0; j<h.cols; j++){
                            uchar &value = v.at<uchar>(i, j);
                            if (value > 0){
                            value = max(10, min(value + desired_value - value_mean, (int)255));

                                }
                            }
                            
                            }
                        }
            
                else if (value_mean < 80 && sat_mean < 80){
                    for(int i=0; i<h.rows; i++){
                        for(int j=0; j<h.cols; j++){
                            uchar &value = v.at<uchar>(i, j);
                            uchar &sat = s.at<uchar>(i, j);
                            if (value > 0){
                            value = max(10, min(value + desired_value - value_mean, (int)255));
                            sat = max(10, min(sat + desired_sat - sat_mean, (int)255));
                                }
                            }
                        }
                }

                else {
                    s.setTo(desired_sat, v > 0);

                    for(int i=0; i<h.rows; i++){
                        for(int j=0; j<h.cols; j++){
                            uchar &value = v.at<uchar>(i, j);
                            if (value > 0){
                            value = max(10, min(value + desired_value - value_mean, (int)255));
                                }
                            }
                        }
                }
                }


            else if (desired_value >= 80 && desired_sat < 80) {
                
                  if (sat_mean < 80 && value_mean >= 80 ){
                    for(int i=0; i<h.rows; i++){
                        for(int j=0; j<h.cols; j++){
                            uchar &value = v.at<uchar>(i, j);
                            uchar &sat = s.at<uchar>(i, j);
                            if (value > 0){
                                sat = max(10, min(sat + desired_sat - sat_mean, (int)255));
                                value = max(10 ,min(desired_value, (int)255));
                                }
                            }
                            }
                        }
                else{
                    
                for(int i=0; i<h.rows; i++){
                    for(int j=0; j<h.cols; j++){
                        uchar &value = v.at<uchar>(i, j);
                        uchar &sat = s.at<uchar>(i, j);
                        if (value > 0){
                            //sat = max(1, min((sat * 40)/255 + desired_sat-20, (int)255));
                            sat = max(10, min(sat + desired_sat - sat_mean, (int)255));

                            value = max(10 ,min(desired_value, (int)255));
                            }
                        }
                    }
                }
                }

            else if (desired_value < 80 && desired_sat >= 80) {
                    
                  if (sat_mean < 80){
                    for(int i=0; i<h.rows; i++){
                        for(int j=0; j<h.cols; j++){
                            uchar &value = v.at<uchar>(i, j);
                            uchar &sat = s.at<uchar>(i, j);
                            if (value > 0){
                                sat = max(10,min(sat + desired_sat /2, (int)255));
                            value = max(10, min(value + desired_value - value_mean, (int)255));
                                }
                            }
                            
                            }
                        }
                else{
                        
                for(int i=0; i<h.rows; i++){
                    for(int j=0; j<h.cols; j++){
                        uchar &value = v.at<uchar>(i, j);
                        if (value > 0){
                            value = max(10, min(value + desired_value - value_mean, (int)255));
                            //cout << desired_value - value_mean;
                            }
                        }
                    }
                }
                }

            else if (desired_value < 80 && desired_sat < 80) {

                for(int i=0; i<h.rows; i++){
                    for(int j=0; j<h.cols; j++){
                        uchar &value = v.at<uchar>(i, j);
                        uchar &sat = s.at<uchar>(i, j);
                        if (value > 0){
                            value = max(10, min(value + desired_value - value_mean, (int)255));
                            }
                        }
                    }
                }
            
            
            merge(hsv_vec, result);
            cvtColor(result, result, COLOR_HSV2BGR);
            
            cvtColor(result, result_grey, COLOR_BGR2GRAY);

            threshold(result_grey, result_grey, 0, 255, THRESH_BINARY_INV);

            bitwise_and(img0, img0, result_2, result_grey);

            final_image = result + result_2;

            img0 = final_image;

            //imshow( "result2", final_image);
            }

        imwrite(outputPath, img0);
          }
    catch (const std::exception& e) {
        cv::Mat img0 = cv::imread(inputImagePath);
        imwrite(outputPath, img0);
  }
    }
}
