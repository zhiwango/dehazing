# Dehazing
This repository is used to publish the code of my image dehazing paper.  
The algorithm can set the parameters below:
```
block: the kernel size of twice minimum filter, before get the dark channel.
morph_size: the kernel size of Morphological Transformations
cirle_wrong_point: circle the wrong pixel of airlight  
```
# Dependency
This algorithm is basesd on C++ and OpenCV.
Install OpenCV from source code, more informatino can be found 
[here](./document/install_opencv.md).
# Sample dataset
Download some sample datasets to test the algorithm.

[Benchmark images](http://kaiminghe.com/cvpr09/images.rar)
# Run the program
```
g++ dehaze.cpp dehaze.h `pkg-config --cflags --libs opencv4` -o dehaze
./dehaze input.bmp
```
# Paper
```
@article{2019TJSKE-D-19-00004,
  title={Research on Haze Removal for Autonomous Car},
  author={Zhi, Wang and Daishi, Watabe},
  journal={Transactions of Japan Society of Kansei Engineering},
  volume={18},
  number={6},
  pages={417-421},
  year={2019},
  doi={10.5057/jjske.TJSKE-D-19-00004}
}
```
