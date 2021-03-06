#include<opencv2/opencv.hpp>
#include<time.h>
#include <fstream>

using namespace cv;
using namespace std;

Mat calcYchannel(Mat &src)
{
	Mat image = Mat::zeros(src.rows, src.cols, CV_8UC1);
	Mat tmp = src.clone();
	cvtColor(tmp, tmp, COLOR_BGR2GRAY);
	vector<Mat>planes;
	split(tmp, planes);
	image = planes.at(0);
	return image;
}

int calcAirlight(Mat &src, int block = 7, bool cirle_wrong_point = false, int MORPH_SIZE = 15, bool save_buf = false, bool save_bufwithmorph = false, bool save_compareimg = false)
{
	double minVal = 0;
	double maxVal = 0;

	Mat rgbmin(src.rows, src.cols, CV_8UC1);
	for (int m = 0; m<src.rows; m++)
	{
		for (int n = 0; n<src.cols; n++)
		{
			Vec3b intensity = src.at<Vec3b>(m, n);
			rgbmin.at<uchar>(m, n) = min(min(intensity.val[0], intensity.val[1]), intensity.val[2]);
		}
	}
	if (rgbmin.empty()){
		printf("Can not load the RGB_min_image.\n");
		return -1;
	}
	Mat darkchannel(Size(src.rows, src.cols), CV_8UC1); // Create a darkchannel image used for next time of minimum filter
	Rect ROI_rect; // Define the ROI size with block, do the twice minimum filter in this block.
	for (int i = 0; i < src.rows / block; i++)
	{
		for (int j = 0; j < src.cols / block; j++)
		{
			ROI_rect.x = i*block;
			ROI_rect.y = j*block;
			ROI_rect.width = block;
			ROI_rect.height = block;
			Mat roi = rgbmin(Rect(ROI_rect.y, ROI_rect.x, ROI_rect.width, ROI_rect.height));
			//Mat roi = src(Range(ROI_rect.x, ROI_rect.x + ROI_rect.width),Range(ROI_rect.y, ROI_rect.y + ROI_rect.height));
			//printf("(%d,%d)", i, j);
			Mat dst_roi = darkchannel(ROI_rect);
			minMaxLoc(roi, &minVal, &maxVal);
			//("%.2f\n", minVal);
			roi.setTo(minVal);
			roi.copyTo(dst_roi);
		}
	}
	transpose(darkchannel, darkchannel); // This is dark channel, but always with some edges.
	// cut off the edge
	int bottom = darkchannel.rows%block;
	int right = darkchannel.cols%block;
	darkchannel = darkchannel(Range(0, darkchannel.rows - bottom), Range(0, darkchannel.cols - right));
	// Here we had done the twice minimum filter on the input image.
	int height = darkchannel.rows;
	int width = darkchannel.cols;
	int some_pixel = height * width * 0.90; // 90% pixels in dark channel

	vector <int> vect_sortPixel;

	for (int i = 0; i < darkchannel.rows*darkchannel.cols; i++)
	{
		vect_sortPixel.push_back(darkchannel.data[i]);
	}
	std::sort(vect_sortPixel.begin(), vect_sortPixel.end()); // Get all of the pixel and sort them.

	int th = vect_sortPixel[some_pixel];
	// Make a threshold image.
	Mat buf = darkchannel.clone();
	for (int i = 0; i < darkchannel.rows; i++)
	{
		for (int j = 0; j < darkchannel.cols; j++)
		{
			if (darkchannel.at<uchar>(i, j) < th)  // If these pixels are blong to 90% pixels, set the color to black
			{
				buf.at<uchar>(i, j) = 0;
			}
			else
			{
				buf.at<uchar>(i, j) = 255; // If these pixels are blong to 10% pixels, set the color to white
			}
		}

	}

	namedWindow("Threshold Image without morphology");
	imshow("Threshold Image without morphology", buf);
	//save image withour morphology
	if (save_buf == true)
	{
		imwrite("buf_nomorphology.bmp", buf);
	}
	// There is still some pixel(not the airlight area) in threshold image.
	Mat element = getStructuringElement(MORPH_RECT, Size(MORPH_SIZE, MORPH_SIZE));
	morphologyEx(buf, buf, MORPH_OPEN, element);

	//return to Y channel to find the pixel.
	Mat y_channel = get_Ychannel(src);
	Mat tmp = src.clone();

	// By set True to circl the wrong point in y channel image.
	if (cirle_wrong_point == true)
	{
		double minDC_wrong_point, maxDC_wrong_point;
		Point minLoc_wrong_point, maxLoc_wrong_point;
		minMaxLoc(y_channel, &minDC_wrong_point, &maxDC_wrong_point, &minLoc_wrong_point, &maxLoc_wrong_point);
		circle(tmp, maxLoc_wrong_point, 5, CV_RGB(255, 0, 0), 2);
	}

	int MAX_I = 0; // Define the value of airlight
	Point maxLoc(0, 0); // Define the location of airlight
	for (int i = 0; i < darkchannel.rows; i++)
	{
		for (int j = 0; j < darkchannel.cols; j++)
		{
			if (buf.at<uchar>(i, j) == 255) // If the pixel is white in threshold image (means in airlight area)
			{
				if (y_channel.at<uchar>(i, j) > MAX_I) // If this pixel in Y channel of YUV color is bigger than A, update A.
				{
					MAX_I = y_channel.at<uchar>(i, j);
					maxLoc.x = j;
					maxLoc.y = i;
				}
			}
		}
	}
	namedWindow("Threshold Image");
	imshow("Threshold Image", buf);
	//save image with morphology
	if (save_bufwithmorph == true)
	{
		imwrite("buf.bmp", buf);
	}
	circle(tmp, maxLoc, 5, CV_RGB(0, 255, 0), 2);
	//save image that include two circled pixel
	if (save_compareimg == true)
	{
		imwrite("compare_point.bmp", tmp);
	}
	namedWindow("The position of A");
	imshow("The position of A", tmp);

	//print out information.
	cout << "The coordinate of pixel is " << maxLoc << endl;
	cout << "The value of brightest pixel is " << MAX_I << endl;
	printf("The mask size of dark channel is: %d x %d.\n", block, block);
	printf("THe mask size of morphylogy is: %d x %d.\n", MORPH_SIZE, MORPH_SIZE);
	return MAX_I;
}

double ave_pixel(Mat &src)
{
	double minVal, maxVal;
	minMaxLoc(src, &minVal, &maxVal);
	double ave = (minVal + maxVal) / 2;
	return ave;
}

void GammaCorrection(Mat& src, Mat& dst, float fGamma)
{
	CV_Assert(src.data);
	// accept only char type matrices
	CV_Assert(src.depth() != sizeof(uchar));
	// build look up table
	unsigned char lut[256];
	for (int i = 0; i < 256; i++)
	{
		lut[i] = saturate_cast<uchar>(pow((float)(i / 255.0), fGamma) * 255.0f);
	}
	dst = src.clone();
	const int channels = dst.channels();
	switch (channels)
	{
	case 1:
	{
		MatIterator_<uchar> it, end;
		for (it = dst.begin<uchar>(), end = dst.end<uchar>(); it != end; it++)
			//*it = pow((float)(((*it))/255.0), fGamma) * 255.0;
			*it = lut[(*it)];
		break;
	}
	case 2:
	{
		MatIterator_<Vec3b> it, end;
		for (it = dst.begin<Vec3b>(), end = dst.end<Vec3b>(); it != end; it++)
		{
			(*it)[0] = lut[((*it)[0])];
			(*it)[1] = lut[((*it)[1])];
			(*it)[2] = lut[((*it)[2])];
		}
		break;
	}
	}
}

Mat calcTransmission(Mat src, Mat Mmed, int a)
{
	Mat transmission_map = Mat::zeros(src.rows, src.cols, CV_8UC3);
	double m, p, q, k;
	m = ave_pixel(src) / 255; // Use m to determin the haze is more or less
	p = 1.3; // Set this value by experiment
	q = 1 + (m - 0.5); // Value q is decided by value m, if m is big and q will be bigger to remove more haze. <- Auto-tunning parameter
	k = min(m*p*q, 0.95);// 
	transmission_map = 255 * (1 - k*Mmed / a);
	GammaCorrection(transmission_map, transmission_map, 1.3 - m);
	printf("m=%f\n", m);
	//imshow("transmission_map", transmission_map);
	//imwrite("trasnmission.bmp", transmission_map);
	return transmission_map;
}

Mat dehazing(Mat &src, Mat &t, int a)
{
	double tmin = 0.1;
	double tmax;
	Scalar inttran;
	Vec3b intsrc;
	Mat dehazed = Mat::zeros(src.rows, src.cols, CV_8UC3);
	for (int i = 0; i<src.rows; i++)
	{
		for (int j = 0; j<src.cols; j++)
		{
			inttran = t.at<uchar>(i, j);
			intsrc = src.at<Vec3b>(i, j);
			tmax = (inttran.val[0] / 255) < tmin ? tmin : (inttran.val[0] / 255);
			for (int k = 0; k<3; k++)
			{
				dehazed.at<Vec3b>(i, j)[k] = abs((intsrc.val[k] - a) / tmax + a) > 255 ? 255 : abs((intsrc.val[k] - a) / tmax + a);
			}
		}
	}
	return dehazed;
}

int main()
{
	int airlight;
	Mat img, y_channel, y_channel_median, t_map, dst;
	string ImgName;
	ifstream fin("./images/LIST.TXT");
	int img_index = 1;
	while (getline(fin, ImgName))
	{
		ImgName = "./images/" + ImgName;
		cout << "Processing:" << ImgName << endl;
		img = imread(ImgName);
		if (img.empty()){
			printf("Can not load the source picture.\n");
			return -1;
		}
		//namedWindow("Input");
		//imshow("Input", img);
		y_channel = calcYchannel(img);
		medianBlur(y_channel, y_channel_median, 5);
		airlight = calcAirlight(img);
		t_map = calcTransmission(img, y_channel_median, airlight);
		dst = dehazing(img, t_map, airlight);
		char save_file[20];
		sprintf(save_file, "./dehazed/%d_dehazed.bmp", img_index);
		img_index++;
		//namedWindow("Dehazing");
		//imshow("Dehazing", dst);
		imwrite(save_file, dst);
		cout << "image " << img_index - 1 << " has been saved." << endl;
	}
	waitKey(0);
	return 0;
}