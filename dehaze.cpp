#include "dehaze.h"

int block_size = 5;
int morphology_transorm_kernel_size = 15;

bool save_buf = false;
bool save_bufwithmorph = false;
bool save_compare_img = false;
bool is_circle_wrong_point = true;

int main(int argc, char **argv)
{
	int airlight;
	Mat img, y_channel, y_channel_median, t_map, dst;
	img = imread(argv[1]);

	if (img.empty()){
		printf("Can not load the picture.\n");
		return -1;
	}
	namedWindow("Input");
	imshow("Input", img);

	y_channel = calcYchannel(img);
	medianBlur(y_channel, y_channel_median, 5);
	airlight = calcAirlight(img, block_size, is_circle_wrong_point, morphology_transorm_kernel_size, save_buf, save_bufwithmorph, save_compare_img);
	t_map = calcTransmission(img, y_channel_median, airlight);
	dst = dehazing(img, t_map, airlight);
	namedWindow("Dehazing");
	imshow("Dehazing", dst);
	//imwrite("dehaze.bmp", dst);
	waitKey(0);
	destroyAllWindows();
	return 0;
}