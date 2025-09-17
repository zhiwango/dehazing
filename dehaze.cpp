#include "dehaze.h"

int main(int argc, char **argv)
{
	// 参数检查
    if (argc < 3) {
        cout << "Usage:\n"
             << "  " << argv[0] << " image <imagePath>\n"
             << "  " << argv[0] << " video <videoPath> <outputVideo>\n"
             << "  " << argv[0] << " batch <inputDir> <outputDir>\n";
        return -1;
    }
	string mode = argv[1];

	if (mode == "image")
	{
        processImage(argv[2]);
    }
	else if (mode == "video")
	{
        processVideo(argv[2], argv[3]);
    }
	else if (mode == "batch")
	{
        batchProcessVideos(argv[2], argv[3]);
    }
	else
	{
        cerr << "Unknown mode: " << mode << endl;
    }
	return 0;
}