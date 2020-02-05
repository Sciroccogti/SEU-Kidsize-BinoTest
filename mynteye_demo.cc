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

int main(int argc, char *argv[])
{
    const int width = 640;
    const int height = 400;
    const int channel = 1;

    struct yolo
    {
        int x = 50;
        int y = 50;
        int w = 100;
        int h = 100;
    } ball_dets;

    std::shared_ptr<mynteye::API> api;
    api = API::Create(argc, argv);

    if (!api)
        return 1;

    // opencv为保证左目的参考点能在右目中找到，会跳过左侧nDisp - 1列像素
    // The max disparity must be a positive integer divisible by 16
    // numberOfDisparities，视差窗口，即最大视差值与最小视差值之差
    const int nDisp = ((width / 8) + 15) & -16; // 80
    // The block size must be a positive odd number, normally >= 3 and <= 11
    // SADWindowSize
    const int SADWin = 5;
    // preFilterCap，预处理滤波器的截断值，opencv里都设63（？？）
    const int preFC = 63;
    // penalty on the disparity change by plus or minus 1 between neighbor pixels
    const int P1 = 8 * channel * SADWin * SADWin; // 200
    // penalty on the disparity change by more than 1 between neighbor pixels
    const int P2 = 4 * P1; // 800

    // cv::Ptr<cv::StereoBM> bm = cv::StereoBM::create(16,9);
    cv::Ptr<cv::StereoSGBM> sgbm = cv::StereoSGBM::create(0, nDisp, SADWin, P1, P2,
                                                          1, preFC, 10, 100, 32,
                                                          cv::StereoSGBM::MODE_SGBM);

    // cv::Stitcher stit = cv::Stitcher::createDefault(true);

    // bool ok;
    // auto &&request = api->SelectStreamRequest(&ok); // ask user to select a stream
    const StreamRequest request = StreamRequest(2 * width, height, Format::YUYV, 60);

    // if (!ok) return 1;
    api->ConfigStreamRequest(request);
    // std::cout << request << std::endl;

    api->EnableStreamData(Stream::LEFT_RECTIFIED);
    api->EnableStreamData(Stream::RIGHT_RECTIFIED);
    // api->SetDisparityComputingMethodType(DisparityComputingMethod::SGBM);
    // api->EnableStreamData(Stream::DISPARITY_NORMALIZED);
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
            double t_c = cv::getTickCount() / cv::getTickFrequency();
            fps = 1.0 / (t_c - t);
            printf("\r%02.2f", fps);
            t = t_c;
            cv::rectangle(left_data.frame, cv::Point(ball_dets.x, ball_dets.y),
                          cv::Point(ball_dets.x + ball_dets.w, ball_dets.y + ball_dets.h),
                          cv::Scalar(0, 0, 255));

            // cv::Rect left_rect()
            cv::Mat mask_left = cv::Mat::zeros(left_data.frame.size(), CV_8UC1);
            cv::Rect roi_left;
            roi_left.x = (ball_dets.x - SADWin / 2) >= 0 ? (ball_dets.x - SADWin / 2) : 0;
            roi_left.y = (ball_dets.y - SADWin / 2) >= 0 ? (ball_dets.x - SADWin / 2) : 0;
            roi_left.width = ball_dets.w + SADWin;
            roi_left.width = (roi_left.width + roi_left.x) < width ? roi_left.width : (width - roi_left.width);
            roi_left.height = ball_dets.h + SADWin;
            roi_left.height = (roi_left.height + roi_left.y) < height ? roi_left.height : (height - roi_left.height);
            mask_left(roi_left).setTo(255);
            left_data.frame.copyTo(img, mask_left);
            cv::imshow("left", img);

            cv::Mat mask_right = cv::Mat::zeros(right_data.frame.size(), CV_8UC1);
            cv::Rect roi_right = roi_left;
            roi_right.width = width - roi_right.x;
            mask_right(roi_right).setTo(255);
            cv::Mat img2;
            right_data.frame.copyTo(img2, mask_right);
            cv::imshow("right", img2);
            // cv::hconcat(left_data.frame, right_data.frame, img);

            // cv::Rect left_rect(0, 0, width * 2 / 3, height);
            // cv::Rect right_rect(width / 3, 0, width * 2 / 3, height);
            // cv::Mat img_l = left_data.frame(left_rect);
            // cv::Mat img_r = right_data.frame(right_rect);
            // cv::hconcat(left_data.frame, img_r, img);
            // std::vector<cv::Mat> imgs;
            // imgs.push_back(left_data.frame);
            // imgs.push_back(right_data.frame);
            // cv::Stitcher::Status status = stit.stitch(imgs, img);
            // if (status == cv::Stitcher::OK)
            // {
            // cv::imshow("frame", img);
            // }
            // imgs.clear();

            sgbm->compute(img, img2, disp);
            disp.convertTo(disp8, CV_8U, 255 / (nDisp * 16.));
            cv::imshow("disp", disp8);
        }

        // if (!disp_norm_data.frame.empty())
        // {
        //     double t_c = cv::getTickCount() / cv::getTickFrequency();
        //     fps = 1.0 / (t_c - t);
        //     printf("%02.2f\n", fps);
        //     t = t_c;
        //     cv::imshow("disparity_normalized", disp_norm_data.frame); // CV_8UC1
        // }

        char key = static_cast<char>(cv::waitKey(1));
        if (key == 27 || key == 'q' || key == 'Q')
        { // ESC/Q
            break;
        }
    }

    api->Stop(Source::VIDEO_STREAMING);
    return 0;
}
