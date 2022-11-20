// https://www.geeksforgeeks.org/how-to-install-opencv-in-c-on-linux/

#include "opencv2/imgproc.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/videoio.hpp"
#include <iostream>
using namespace cv;
using namespace std;

const int max_value_H = 360 / 2;
const int max_value = 255;
const String window_detection_name = "Ayse";
int H_val = 0;

static void on_H_val_thresh_trackbar(int, void *)
{
    H_val = min(max_value_H-1, H_val);
    setTrackbarPos("H", window_detection_name, H_val);
}


int main(int argc, char* argv[])
{

	//Reading the image in the source files part 
	Mat img = imread("/home/yoy/Desktop/furniture_customization_for_sugar_daddy/couch.jpg");
	imshow("original image", img);

	//If the image is empty, error pops up
	if (img.empty())
	{
		std::cout << "Could not read the image: ";
		return 1;
	}


	//BGR -> HSV changing part
	Mat HSVImage;
	cvtColor(img, HSVImage, cv::COLOR_BGR2HSV);


    namedWindow(window_detection_name);
    createTrackbar("H", window_detection_name, &H_val, max_value_H, on_H_val_thresh_trackbar);

	//changing Hue value

	Mat hsv = HSVImage.clone();
	vector<Mat> hsv_vec;
	split(hsv, hsv_vec); //this is an opencv function
	Mat& H = hsv_vec[0];
	Mat& S = hsv_vec[1];
	Mat& V = hsv_vec[2];
	Mat finalImage;


    while (true) {

	H = H_val; // H is between 0-180 in OpenCV
	merge(hsv_vec, hsv);
	HSVImage = hsv; // according to your code


	//showing HSV image Notice: imshow always renders in BGR space
	cvtColor(HSVImage, finalImage, cv::COLOR_HSV2BGR);
	imshow(window_detection_name, finalImage);

    char key = (char) waitKey(30);
    if (key == 'q' || key == 27)
    {
        break;
    }
    } 

	return 0;
}
