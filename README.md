# Dehazing
This repository is used to publish the code, that usd in my image dehazing paper.  
<img height="400" src="./others/input.jpg" width="300"/>
<img height="400" src="./others/output.jpg" width="300"/>  
The C++ algorithm can set the parameters below:
```
blockSize: the kernel size of twice minimum filter, before get the dark channel.
morphSize: the kernel size of Morphological Transformations
cirleWrongPoint: circle the wrong pixel of airlight  
```
Also, there is a python source code.
To calculate the dark channel, I used a erode calculation, same as the twice minimum filter.  
  
![erode](./others/erode_formula.png)
# Dependency
This algorithm is basesd on C++ and OpenCV.
Install OpenCV from source code, more informatino can be found 
[here](./others/install_opencv.md).
# Sample dataset
Download some sample datasets to test the algorithm.

[Benchmark images](http://kaiminghe.com/cvpr09/images.rar)
# Run the program
## C++
```
g++ dehaze.cpp dehaze.h -o dehazing `pkg-config --cflags --libs opencv4`
./dehaze input.bmp
```
## Python
```
python3 dehaze.py input_file file_type
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
