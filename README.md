# SEU-Kidsize-BinoTest

为ROS版本Robocup代码做的双目测试

目前ROS上的OpenCV版本默认为opencv3，非常舒适。

## api中的效果

*   sgbm方式速度极慢，仅计算视差图时fps都只有4左右，但是效果似乎不错；
*   bm方式速度很快，仅计算视差图的话fps在20上下波动，不过图块连片，目测效果很差。

因此考虑进行改写，由于本项目仅需对yolo框选出的物体进行测距，因此可以减少视差计算范围，加快速度。

目前计划使用sgbm算法。

## 参考教程

以下均为[Eason.wxd](https://me.csdn.net/App_12062011)的系列博客：
* [双目测距（一）：图像获取与单目标定](https://blog.csdn.net/App_12062011/article/details/52032563)
* [双目测距（二）：双目标定与矫正](https://blog.csdn.net/App_12062011/article/details/52032667)
* [**双目测距（三）：立体匹配**](https://blog.csdn.net/App_12062011/article/details/52032935)
* [双目测距（四）：罗德里格斯变换](https://blog.csdn.net/App_12062011/article/details/52033328)
* [双目测距（五）：匹配算法对比](https://blog.csdn.net/App_12062011/article/details/52034855)
* [双目测距（六）：三维重建及UI显示](https://blog.csdn.net/App_12062011/article/details/52034906)

### 局部匹配准则

以 **SAD** （Sum of Absolute Differences）为例：

SAD 算法：SAD算法是一种最简单的匹配算法，用公式表示为：

SAD(u,v) = Sum{|Left(u,v) - Right(u,v)|}  选择最小值

此种方法就是以左目图像的源匹配点为中心，定义一个窗口D，其大小为（2m+1） (2n+1)，
统计其窗口的灰度值的和，然后在右目图像中逐步计算其左右窗口的灰度和的差值，
最后搜索到的差值最小的区域的中心像素即为匹配点。

### preFilterCap 匹配图像预处理

两种立体匹配算法都要先对输入图像做预处理，OpenCV源码中中调用函数
`static void prefilterXSobel(const Mat& src, Mat& dst, int preFilterCap)`，
参数设置中preFilterCap在此函数中用到。
函数步骤如下，作用主要有两点：对于无纹理区域，能够排除噪声干扰；
对于边界区域，能够提高边界的区分性，利于后续的匹配代价计算：
1.  先利用水平Sobel算子求输入图像x方向的微分值Value；
    *   如果Value<-preFilterCap, 则Value=0;
    *   如果Value>preFilterCap,则Value=2*preFilterCap;
    *   如果Value>=-preFilterCap &&Value<=preFilterCap,则Value=Value+preFilterCap
2.  输出处理后的图像作为下一步计算匹配代价的输入图像。

## TODO

双目图像合并为一张再传给yolo，来达到扩展视野的效果。