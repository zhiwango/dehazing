//https://doi.org/10.5057/jjske.TJSKE-D-19-00004

#include "dehaze.h"

int block = 5;
int morph_size = 30;

bool save_buf = false;
bool save_bufwithmorph = false;
bool save_compare_img = false;
bool cirle_wrong_point = true;

int main()
{
	int airlight;
	Mat img, y_channel, y_channel_median, t_map, dst;
	img = imread("./img/train.bmp");

	if (img.empty()){
		printf("Can not load the picture.\n");
		return -1;
	}
	namedWindow("Input");
	imshow("Input", img);

	y_channel = get_Ychannel(img);
	medianBlur(y_channel, y_channel_median, 5);
	airlight = calculate_airlight_in_dark_channel(img, block, cirle_wrong_point, morph_size, save_buf, save_bufwithmorph, save_compare_img);
	t_map = transmission(img, y_channel_median, airlight);
	dst = getDehazed(img, t_map, airlight);
	namedWindow("Dehazing");
	imshow("Dehazing", dst);
	//imwrite("dehaze.bmp", dst);
	waitKey(0);
	destroyAllWindows();
	return 0;
}