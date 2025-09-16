#include "dehaze.h"

int main(int argc, char **argv)
{
	// 参数检查
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " <image_path>" << endl;
        return -1;
    }

	// 参数配置
    constexpr int blockSize = 5;
    constexpr int morphSize = 15;
    constexpr bool saveBuf = false;
    constexpr bool saveBufWithMorph = false;
    constexpr bool saveCompareImg = false;
    constexpr bool circleWrongPoint = true;

	// 读取图像
	Mat input = imread(argv[1]);
	if (input.empty()){
		cerr << "Error: cannot load image from path: " << argv[1] << endl;
		return -1;
	}
	imshow("Input", input);

	// Step1. 计算 Y 通道 & 中值滤波
	Mat yChannel = calcYchannel(input);
	Mat yChannelMedian;
	medianBlur(yChannel, yChannelMedian, 5);

	// Step2. 估计大气光
	int airlight = calcAirlight(input, blockSize, morphSize, circleWrongPoint, saveBuf, saveBufWithMorph, saveCompareImg);

	// Step3. 计算透射率图
	Mat transmission = calcTransmission(input, yChannelMedian, airlight);

	// Step4. 图像去雾
	Mat dehazed = dehazing(input, transmission, airlight);
	imshow("Dehazing", dehazed);
	// 保存结果（可选）
	//imwrite("dehaze.bmp", dehazed);

	waitKey(0);
	destroyAllWindows();
	return 0;
}