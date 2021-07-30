#include "dehaze.h"

int block = 5;
int morph_size = 15;

bool save_buf = false;
bool save_bufwithmorph = false;
bool save_compare_img = false;
bool cirle_wrong_point = true;

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
	airlight = calcAirlight(img, block, cirle_wrong_point, morph_size, save_buf, save_bufwithmorph, save_compare_img);
	t_map = calcTransmission(img, y_channel_median, airlight);
	dst = dehazing(img, t_map, airlight);
	namedWindow("Dehazing");
	imshow("Dehazing", dst);
	//imwrite("dehaze.bmp", dst);
	waitKey(0);
	destroyAllWindows();
	return 0;
}