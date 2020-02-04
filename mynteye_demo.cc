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
#include <opencv2/highgui/highgui.hpp>

#include "mynteye/api/api.h"
#include "mynteye/device/device.h"

MYNTEYE_USE_NAMESPACE

int main(int argc, char *argv[])
{
    std::cout << ((CV_8UC1) >> CV_CN_SHIFT) + 1 << std::endl;

    std::shared_ptr<mynteye::API> api;
    api = API::Create(argc, argv);
    // try
    // {
    //     static std::shared_ptr<mynteye::Device> BiDevice = mynteye::Device::Create("MYNT-EYE-S2110", device);
    // }
    // catch(const std::exception& e)
    // {
    //     std::cerr << e.what() << '\n';
    // }

    if (!api)
        return 1;

    // bool ok;
    // auto &&request = api->SelectStreamRequest(&ok); // ask user to select a stream
    const mynteye::StreamRequest request = mynteye::StreamRequest(1280, 400, mynteye::Format::YUYV, 30);

    // if (!ok) return 1;
    api->ConfigStreamRequest(request);
    // std::cout << request << std::endl;

    api->EnableStreamData(Stream::LEFT_RECTIFIED);
    api->EnableStreamData(Stream::RIGHT_RECTIFIED);
    api->SetDisparityComputingMethodType(DisparityComputingMethod::SGBM);
    api->EnableStreamData(Stream::DISPARITY_NORMALIZED);
    api->Start(Source::VIDEO_STREAMING);

    
    api->SetOptionValue(Option::EXPOSURE_MODE, 0); // auto exposure
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

    cv::namedWindow("frame");

    while (true)
    {
        api->WaitForStreams();

        auto &&left_data = api->GetStreamData(Stream::LEFT_RECTIFIED);
        auto &&right_data = api->GetStreamData(Stream::RIGHT_RECTIFIED);
        auto &&disp_norm_data = api->GetStreamData(Stream::DISPARITY_NORMALIZED);

        cv::Mat img;
        if (!left_data.frame.empty() && !right_data.frame.empty())
        {
            // double t_c = cv::getTickCount() / cv::getTickFrequency();
            // fps = 1.0 / (t_c - t);
            // printf("\r%02.2f", fps);
            // t = t_c;
            cv::hconcat(left_data.frame, right_data.frame, img);
            cv::imshow("frame", img);
        }

        if (!disp_norm_data.frame.empty())
        {
            double t_c = cv::getTickCount() / cv::getTickFrequency();
            fps = 1.0 / (t_c - t);
            printf("%02.2f\n", fps);
            t = t_c;
            cv::imshow("disparity_normalized", disp_norm_data.frame); // CV_8UC1
        }

        char key = static_cast<char>(cv::waitKey(1));
        if (key == 27 || key == 'q' || key == 'Q')
        { // ESC/Q
            break;
        }
    }

    api->Stop(Source::VIDEO_STREAMING);
    return 0;
}
