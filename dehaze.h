#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <algorithm>
#include <filesystem>

using namespace cv;
using namespace std;
namespace fs = std::filesystem;

constexpr int airlight = 255;	// 环境光参数，值越小处理后的帧越亮。
constexpr double scale = 1.0;	//原始视频的缩放参数

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
	return transmission;
}

//------------------------------------------------------------
// 图像复原
//------------------------------------------------------------
Mat dehazing(const Mat &src, const Mat &t, int airlight)
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
				double val = (rowSrc[j][c] - airlight) / tVal + airlight;
				rowDst[j][c] = saturate_cast<uchar>(val);
			}
		}
	}
	return dehazed;
}

//------------------------------------------------------------
// 图像去雾处理进程
//------------------------------------------------------------
Mat processFrame(const Mat &frame, int airlight) {
    if (frame.empty()) return frame;

    Mat yChannel, yChannelMedian, transmission, dehazed;
    yChannel = calcYchannel(frame);
    medianBlur(yChannel, yChannelMedian, 5);
    transmission = calcTransmission(frame, yChannelMedian, airlight);
    dehazed = dehazing(frame, transmission, airlight);
    return dehazed;
}

//------------------------------------------------------------
// 处理单张图片
//------------------------------------------------------------
void processImage(const string &imagePath) {
    Mat img = imread(imagePath);
    if (img.empty()) {
        cout << "Can not load the picture: " << imagePath << endl;
        return;
    }

    Mat dehazed = processFrame(img, airlight);

    imshow("Input", img);
    imshow("Dehazing", dehazed);
    imwrite("output.jpg", dehazed);

    waitKey(0);
    destroyAllWindows();
}

//------------------------------------------------------------
// 处理单个视频（实时显示并保存）
//------------------------------------------------------------
void processVideo(const string &videoPath, const string &outputPath) {
    VideoCapture cap(videoPath);
    if (!cap.isOpened()) {
        cout << "Can not open video: " << videoPath << endl;
        return;
    }

    // 原始视频参数
    int frame_width = static_cast<int>(cap.get(CAP_PROP_FRAME_WIDTH) / 2);
    int frame_height = static_cast<int>(cap.get(CAP_PROP_FRAME_HEIGHT) / 2);
    double fps = cap.get(CAP_PROP_FPS);

    // 缩放比例
    int out_width  = static_cast<int>(frame_width * scale);
    int out_height = static_cast<int>(frame_height * scale);

    // Writer 分辨率要和 combined 一致
    int combined_width  = out_width * 2;   // 左右拼接
    int combined_height = out_height;

    // Writer
    VideoWriter writer(outputPath,
                       VideoWriter::fourcc('m','p','4','v'),
                       fps,
                       Size(combined_width, combined_height));

    Mat frame, processed;
    int frameCount = 0;
    double avg_fps = 0.0;

    while (true) {
        cap >> frame;
        if (frame.empty()) break;
        frameCount++;

        // 缩放帧
        resize(frame, frame, Size(out_width, out_height));

        // 开始计时
        auto start = chrono::high_resolution_clock::now();

        // 图像处理
        processed = processFrame(frame, airlight);

        // 结束计时
        auto end = chrono::high_resolution_clock::now();
        chrono::duration<double> elapsed = end - start;
        double fps_now = 1.0 / elapsed.count();

        // 滑动平均
        avg_fps = (avg_fps * (frameCount - 1) + fps_now) / frameCount;

        // 显示 FPS
        string text = "FPS: " + to_string(fps_now).substr(0, 5) + "  Avg: " + to_string(avg_fps).substr(0, 5);
        putText(processed, text, Point(20, 40), FONT_HERSHEY_SIMPLEX, 1.0, Scalar(0, 255, 0), 2);

        // 结合图像
        Mat combined;
        hconcat(frame, processed, combined);

        // 显示结果
        imshow("Original | Dehazing", combined);

        // 保存结果帧
        writer.write(combined);

        // ESC 退出
        if (waitKey(1) == 27) break; // ESC 退出
    }

    cap.release();
    writer.release();
    destroyAllWindows();
}

//------------------------------------------------------------
// 批量处理视频（只保存）
//------------------------------------------------------------
void batchProcessVideos(const string &inputDir, const string &outputDir) {
    fs::create_directories(outputDir);

    for (const auto &entry : fs::directory_iterator(inputDir)) {
        if (entry.is_regular_file()) {
            string path = entry.path().string();
            string outPath = outputDir + "/" + entry.path().filename().string();
            cout << "Processing video: " << path << endl;
            processVideo(path, outPath); // 批量时不显示
        }
    }
}

