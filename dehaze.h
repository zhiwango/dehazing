#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <algorithm>

using namespace cv;
using namespace std;

//------------------------------------------------------------
// 计算 YUV 颜色空间的 Y 通道
//------------------------------------------------------------
Mat calcYchannel(const Mat &src)
{
	Mat yuv, ychannel;
	CV_Assert(!src.empty() && src.type() == CV_8UC3);
	cvtColor(src, yuv, COLOR_BGR2YUV);
	vector<Mat>planes;
	split(yuv, planes);
	return planes[0];
}

//------------------------------------------------------------
// 计算暗通道并估计大气光
//------------------------------------------------------------
int calcAirlight(const Mat &src, int blockSize, int morphSize, bool cirleWrongPoint, bool saveBuf, bool saveBufWithMorph, bool saveCompareImg)
{
	CV_Assert(!src.empty() && src.type() == CV_8UC3);

	// Step 1. 计算每个像素的 RGB 最小值
	Mat rgbMin(src.rows, src.cols, CV_8UC1);

	for (int i = 0; i<src.rows; i++)
	{
		const Vec3b* row = src.ptr<Vec3b>(i);
		uchar* out = rgbMin.ptr<uchar>(i);
		for (int j = 0; j<src.cols; j++)
		{
			out[j] = static_cast<uchar>(std::min({row[j][0], row[j][1], row[j][2]}));
		}
	}
	imshow("rgbmin", rgbMin);

	// Step 2. 基于 blockSize 进行最小滤波，得到暗通道
	Mat darkChannel(src.size(), CV_8UC1, Scalar(0));

	for (int i = 0; i < src.rows; i += blockSize)
	{
		for (int j = 0; j < src.cols; j+= blockSize)
		{
			int h = std::min(blockSize, src.rows - i);
			int w = std::min(blockSize, src.cols - j);

			Rect roi(j, i, w, h);
			double minVal;
			minMaxLoc(rgbMin(roi), &minVal, nullptr);
			darkChannel(roi).setTo(static_cast<uchar>(minVal));
		}
	}
	imshow("darkchannel", darkChannel);

	// Step 3. 阈值分割，选取暗通道中最亮的 10%
	vector<uchar> pixels;
	pixels.assign(darkChannel.datastart, darkChannel.dataend);
	sort(pixels.begin(), pixels.end());

	int th = pixels[static_cast<size_t>(pixels.size() * 0.9)];
	Mat mask = darkChannel >= th;

	imshow("Threshold Image without morphology", mask);
	if (saveBuf) imwrite("buf_nomorphology.bmp", mask);


	// Step 4. 形态学操作去除噪声
	// There is still some pixel(not the airlight area) in threshold image.
	Mat element = getStructuringElement(MORPH_RECT, Size(morphSize, morphSize));
	morphologyEx(mask, mask, MORPH_OPEN, element);


	// Step 5. 找到亮度最高的点作为大气光
	// Return to Y channel to find the pixel.
	Mat yChannel = calcYchannel(src);
	int maxI = 0; // Define the value of airlight
	Point maxLoc(0, 0); // Define the location of airlight

	for (int i = 0; i < mask.rows; i++)
	{
		const uchar* mRow = mask.ptr<uchar>(i);
		const uchar* yRow = yChannel.ptr<uchar>(i);

		for (int j = 0; j < mask.cols; j++)
		{
			if(mRow[j] == 255 && yRow[j] > maxI)
			{
				maxI = yRow[j];
				maxLoc = Point(i, j);
			}
		}
	}

	// Step 6. 可视化
	Mat display = src.clone();
	if(cirleWrongPoint)
	{
		double minVal, maxVal;
		Point minLoc, maxLocWrong;
		minMaxLoc(yChannel, &minVal, &maxVal, &minLoc, &maxLocWrong);
		circle(display, maxLocWrong, 5, Scalar(0, 0, 255), 2);
	}
	circle(display, maxLoc, 5, Scalar(0, 255, 0), 2);

	imshow("Threshold Image", mask);
	imshow("The position of Airlight", display);

	if(saveBufWithMorph) imwrite("buf.bmp", mask);
	if(saveCompareImg) imwrite("compare_point.bmp", display);


	// Print information
	cout << "The coordinate of Airlight is " << maxLoc << endl;
	cout << "The brightest pixel value is " << maxI << endl;
	cout << "The mask size of dark channel is: " << blockSize << " x " << blockSize << "."<<endl;
	cout << "The mask size of morphylogy is: " << morphSize << " x " << morphSize << "."<<endl;
	return maxI;
}

//------------------------------------------------------------
// 计算图像亮度均值
//------------------------------------------------------------
// Calculate the average of two point (brightest pixel and darkest pixel) in source image
// To determine the haze in source image is more or less
double avePixel(const Mat &src)
{
	double minVal, maxVal;
	minMaxLoc(src, &minVal, &maxVal);
	return (minVal + maxVal) / 2.0;
}

//------------------------------------------------------------
// Gamma 矫正
//------------------------------------------------------------
void gammaCorrection(const Mat& src, Mat& dst, float gamma)
{
	CV_Assert(!src.empty());

	// Build look up table
	uchar lut[256];
	for (int i = 0; i < 256; i++)
	{
		lut[i] = saturate_cast<uchar>(pow(i / 255.0, gamma) * 255.0f);
	}

	dst = src.clone();
	for (int i = 0; i < src.rows; i++)
	{
		uchar* row = dst.ptr<uchar>(i);
		for (int j = 0; j < src.cols * src.channels(); j++)
		{
			row[j] = lut[row[j]];
		}
	}
}

//------------------------------------------------------------
// 计算透射率图
//------------------------------------------------------------
Mat calcTransmission(const Mat& src, const Mat& Mmed, int a)
{
	CV_Assert(!src.empty() && !Mmed.empty());

	double m = avePixel(src) / 255.0; // Use m to determin the haze is more or less
	double p = 1.3; // Set this value by experiment
	double q = 1 + (m - 0.5); // Value q is decided by value m, if m is big and q will be bigger to remove more haze. <- Auto-tunning parameter
	double k = min(m * p * q, 0.95);

	Mat transmission = 255 * (1 - k * Mmed / a);
	gammaCorrection(transmission, transmission, 1.3f - static_cast<float>(m));
	cout << "m = " << m << endl;
	return transmission;
}

//------------------------------------------------------------
// 图像复原
//------------------------------------------------------------
Mat dehazing(const Mat &src, const Mat &t, int a)
{
	CV_Assert(!src.empty() && !t.empty());

	Mat dehazed(src.size(), CV_8UC3);
	const double tmin = 0.1;

	for (int i = 0; i < src.rows; i++)
	{
		const Vec3b* rowSrc = src.ptr<Vec3b>(i);
		const uchar* rowT = t.ptr<uchar>(i);
		Vec3b* rowDst  = dehazed.ptr<Vec3b>(i);

		for (int j = 0; j < src.cols; j++)
		{
			double tVal = max(rowT[j] / 255.0, tmin);
			for (int c = 0; c < 3; c++)
			{
				double val = (rowSrc[j][c] - a) / tVal + a;
				rowDst[j][c] = saturate_cast<uchar>(val);
			}
		}
	}
	return dehazed;
}
