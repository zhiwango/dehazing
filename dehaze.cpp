//https://doi.org/10.5057/jjske.TJSKE-D-19-00004

#include "dehaze.h"

int main()
{
	int a;
	Mat img, y_channel, y_channel_median, t_map, dst;
	img = imread("train.bmp");


	if (img.empty()){
		printf("Can not load the picture.\n");
		return -1;
	}
	namedWindow("Input");
	imshow("Input", img);

	y_channel = get_Ychannel(img);
	medianBlur(y_channel, y_channel_median, 5);
	a = Calculate_a_in_dark_channel(img);
	t_map = transmission(img, y_channel_median, a);
	dst = getDehazed(img, t_map, a);
	namedWindow("Dehazing");
	imshow("Dehazing", dst);
	//imwrite("dehaze.bmp", dst);
	waitKey(0);
	destroyAllWindows();
	return 0;
}