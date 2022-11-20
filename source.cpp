#include "opencv2/imgproc.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/videoio.hpp"
#include <iostream>
#include <vector>
#include <dirent.h>

using namespace cv;
using namespace std;

const int max_value_H = 360 / 2;
const int max_value = 255;
const String window_detection_name = "Image";
const String trackbar_window = "Tracker";
int H_val = 0;
bool debug = true;
int S_val,L_val,index_val = 0;
const int* ptr;
int old_index = 999;

vector<string> files = {};

void image_iterator(const char *filepath){
    struct dirent *entry = nullptr;
    DIR *dp = nullptr;
    dp = opendir(filepath);
    if (dp != nullptr) {
        while ((entry = readdir(dp))){
            files.push_back(entry->d_name);
        }
    }

    // First two elemetns are ".." and "." 
    files.erase(files.begin());
    files.erase(files.begin());

	/*
    for (int i = 0; i<files.size() ; i++) {
    cout << files[i] << "\n";
    }
    cout << files.size() << "\n";
	*/
}

static void on_H_val_thresh_trackbar(int, void*)
{
	H_val = min(max_value_H - 1, H_val);
	setTrackbarPos("H", trackbar_window, H_val);
}

static void on_S_val_thresh_trackbar(int, void*)
{
	S_val = min(max_value - 1, S_val);
	setTrackbarPos("S", trackbar_window, S_val);
}

static void on_L_val_thresh_trackbar(int, void*)
{
	L_val = min(max_value - 1, L_val);
	setTrackbarPos("L", trackbar_window, L_val);
}

static void index_change_thresh_trackbar(int, void*)
{
	index_val = min(*ptr - 1, index_val);
	setTrackbarPos("File", trackbar_window, index_val);
}

int main(int argc, char* argv[])
{
	image_iterator("/home/yoy/Desktop/furniture_customization_for_sugar_daddy/pigmentation/images");
	const int index_max = files.size();
    ptr = &index_max;
	Mat hsv;
	Mat HLSImage;
	Mat img, img_temp;
	vector<Mat> hsv_vec, hsv_vec_temp;
	Mat finalImage;

	img_temp = imread("couch.jpg");
	split(img_temp, hsv_vec_temp); //this is an opencv function
	Mat& H = hsv_vec_temp[0];
	Mat& L = hsv_vec_temp[1];
	Mat& S = hsv_vec_temp[2];

	namedWindow(window_detection_name);
	namedWindow(trackbar_window);
	createTrackbar("H", trackbar_window, &H_val, max_value_H, on_H_val_thresh_trackbar);
	createTrackbar("S", trackbar_window, &S_val, max_value, on_S_val_thresh_trackbar);
	createTrackbar("L", trackbar_window, &L_val, max_value, on_S_val_thresh_trackbar);
	createTrackbar("File", trackbar_window, &index_val, index_max, index_change_thresh_trackbar);

	while (true) {

	if (index_val != old_index){
		//Reading the image in the source files part 
		img = imread("images/"+files[index_val]);
		//imshow("original image", img);

		//If the image is empty, error pops up
		if (img.empty())
		{
			std::cout << "Could not read the image: ";
			return 1;
		}


		//BGR -> HSV changing part
		cvtColor(img, HLSImage, cv::COLOR_BGR2HLS);

		//changing Hue value
		//cloning in resultHSV
		hsv = HLSImage.clone();
		split(hsv, hsv_vec); //this is an opencv function
		H = hsv_vec[0];
		L = hsv_vec[1];
		S = hsv_vec[2];
		//Mat S_temp = (S < 15);
		//L = L + L*0.5; //todo: this part is essential for better pigmentation
		//S_temp = 230;

		//S = S + (255-S)*0.8;
		//S = 240;

		cout <<  "Sum of L ____________ " << sum(L)[0] << endl;
		cout << "Size of L ____________ " << L.size[0] * L.size[0] << endl;

		cout << ((cv::sum(L)[0] * 100) / (L.size[0] * L.size[0] * 255));

		old_index = index_val;
	}

		H = H_val; // H is between 0-180 in OpenCV
		if (debug == true) {
			//hsv = (L > 250);
			//S = S_val;
			if (((cv::sum(L)[0] * 100) / (L.size[0] * L.size[0] * 255) > 245)) { //white
				S = 170;

			}
			else if (((cv::sum(L)[0]*100) / (L.size[0] * L.size[0]*255) < 20)) { //background removed ones are black also but solve this
				S = 255;
			}
		}
		else {
			
			hsv = (L < 245);
		}
		merge(hsv_vec, hsv);
		//resultHSV
		HLSImage= hsv; // according to your code


		//showing HSV image Notice: imshow always renders in BGR space
		//resultHSV
		cvtColor(HLSImage, finalImage, cv::COLOR_HLS2BGR);
		imshow(window_detection_name, finalImage);

		char key = (char)waitKey(30);
		if (key == 'q' || key == 27)
		{
			break;
		}
		if (key == 's')
		{
	         cv::imwrite("output_hsl_blue.png", finalImage);
        }
	}

	return 0;
}