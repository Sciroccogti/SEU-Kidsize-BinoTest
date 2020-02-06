// Copyright 2018 Slightech Co., Ltd. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include <stdio.h>
#include <iostream>
#include <vector>
// #include <opencv2/highgui/highgui.hpp>
#include "opencv2/calib3d/calib3d.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/core/utility.hpp"
#include <opencv2/stitching.hpp>

#include "mynteye/api/api.h"
#include "mynteye/device/device.h"

MYNTEYE_USE_NAMESPACE

const int width = 640;
const int height = 400;

int main(int argc, char *argv[])
{

    struct yolo
    {
        int x = 140;
        int y = 50;
        int w = 100;
        int h = 100;
    } ball_dets;

    std::shared_ptr<mynteye::API> api;
    api = API::Create(argc, argv);

    if (!api)
        return 1;

    // bool ok;
    // auto &&request = api->SelectStreamRequest(&ok); // ask user to select a stream
    const StreamRequest request = StreamRequest(2 * width, height, Format::YUYV, 60);

    // if (!ok) return 1;
    api->ConfigStreamRequest(request);
    // std::cout << request << std::endl;

    api->EnableStreamData(Stream::LEFT_RECTIFIED);
    api->EnableStreamData(Stream::RIGHT_RECTIFIED);
    api->SetDisparityComputingMethodType(DisparityComputingMethod::BM);
    // api->EnableStreamData(Stream::DISPARITY_NORMALIZED);
    api->EnableStreamData(Stream::DEPTH);
    api->Start(Source::VIDEO_STREAMING);

    // auto exposure
    api->SetOptionValue(Option::EXPOSURE_MODE, 0);
    // max_gain: range [0,255], default 8
    api->SetOptionValue(Option::MAX_GAIN, 8);
    // max_exposure_time: range [0,655], default 333
    api->SetOptionValue(Option::MAX_EXPOSURE_TIME, 333);
    // desired_brightness: range [1,255], default 122
    api->SetOptionValue(Option::DESIRED_BRIGHTNESS, 122);
    // min_exposure_time: range [0,655], default 0
    api->SetOptionValue(Option::MIN_EXPOSURE_TIME, 0);
    double fps;
    double t = 0.01;
    // std::cout << "fps:" << std::endl;

    // cv::namedWindow("frame");

    // cv::namedWindow("disparity_normalized");

    printf("\n");
    while (true)
    {
        api->WaitForStreams();

        auto &&left_data = api->GetStreamData(Stream::LEFT_RECTIFIED);
        auto &&right_data = api->GetStreamData(Stream::RIGHT_RECTIFIED);
        // auto &&disp_norm_data = api->GetStreamData(Stream::DISPARITY_NORMALIZED);
        // printf("num of channels is %d\n", left_data.frame.channels());
        cv::Mat img, disp, disp8;
        if (!left_data.frame.empty() && !right_data.frame.empty())
        {

            cv::hconcat(left_data.frame, right_data.frame, img);
            cv::imshow("frame", img);
        }

        // if (!disp_norm_data.frame.empty())
        // {
        //     double t_c = cv::getTickCount() / cv::getTickFrequency();
        //     fps = 1.0 / (t_c - t);
        //     printf("%02.2f\n", fps);
        //     t = t_c;
        //     cv::imshow("disparity_normalized", disp_norm_data.frame); // CV_8UC1
        // }

        auto &&depth_data = api->GetStreamData(Stream::DEPTH);
        if (!depth_data.frame.empty())
        {
            // double t_c = cv::getTickCount() / cv::getTickFrequency();
            // fps = 1.0 / (t_c - t);
            // printf("%02.2f\n", fps);
            // t = t_c;
            cv::convertScaleAbs(depth_data.frame, disp, 7000.0 / 65535); // 截取5m以内
            cv::applyColorMap(disp, disp8, cv::COLORMAP_JET);
            cv::Rect roi(ball_dets.x, ball_dets.y, ball_dets.w, ball_dets.h);
            cv::rectangle(disp8, roi, cv::Scalar(255, 255, 255), 2);
            cv::imshow("depth_real", disp8); // CV_16UC1
            cv::Mat depth_cop;
            depth_data.frame(roi).convertTo(depth_cop, CV_32F);
            cv::Mat depth_cop2 = depth_cop.reshape(1, 1);
            // cv::Mat depth_cop2 = depth_cop.clone();
            // printf("%d %d\n", depth_cop.rows, depth_cop.cols);
            std::vector<float> depth_vector = (std::vector<float>)(depth_cop2);
            // printf("%d\n", depth_vector.size());
            // depth_cop.resize(ball_dets.w * ball_dets.h);
            cv::Mat labels;
            std::vector<int> centers;
            // kmeans 只接收CV_32F
            cv::kmeans(depth_vector, 3, labels, cv::TermCriteria(cv::TermCriteria::EPS + cv::TermCriteria::COUNT, 5, 1.0), 3, cv::KMEANS_PP_CENTERS, centers);
            printf("%d %d %d\n", centers[0], centers[1], centers[2]);
            int i, count[3] = {0, 0, 0};
            for (i = 0; i < ball_dets.w * ball_dets.h; i++)
            {
                count[labels.data[i]]++;
            }
            int maxIDX;
            if(count[0] >= count[1] && count[0] >= count[2])
            {
                maxIDX = 0;
            }
            else if(count[1] >= count[2])
            {
                maxIDX = 1;
            }
            else
            {
                maxIDX = 2;
            }
            
            // // int var = Cluster(depth_cop, 7000, 0);
            printf("distance = %d mm\n", centers[maxIDX]);
            // break;
            // printf("mean = %d\n", (int)cv::mean(depth_data.frame(roi))[0]);
            // cv::minMaxIdx(disp, &min, &max);
            // for (int i = 0; i < height; i++)
            // {
            //     for (int j = 0; j < width; j++)
            //     {
            //         tmp = depth_data.frame.at<uchar>(i, j);
            //         max = tmp > max ? tmp : max;
            //         min = tmp < min ? tmp : min;
            //     }
            // }
            // printf("%lf %lf\n", min, max);
        }

        cv::Rect roi(ball_dets.x, ball_dets.y, ball_dets.w, ball_dets.h);

        // printf("%3d\n", tmp);
        char key = static_cast<char>(cv::waitKey(1));
        if (key == 27 || key == 'q' || key == 'Q')
        { // ESC/Q
            break;
        }
    }

    api->Stop(Source::VIDEO_STREAMING);
    return 0;
}
