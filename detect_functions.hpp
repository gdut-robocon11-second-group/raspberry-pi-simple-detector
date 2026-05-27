#pragma once

#include <opencv2/opencv.hpp>
#include <vector>
#include <string>
#include <optional>

class Detector {
public:
    Detector(cv::VideoCapture& cap) : cap_(cap) {}

    std::optional<std::string> detectQRCode() {
        cv::Mat frame;
        if (cap_.read(frame)) {
            return std::nullopt;
        }

        if (frame.empty()) {
            return std::nullopt;
        }

        cv::QRCodeDetector qrDecoder;
        std::string data;
        try {
            data = qrDecoder.detectAndDecode(frame);
        } catch (const std::exception& e) {
            return std::nullopt;
        }

        if (data.empty()) {
            return std::nullopt; // No QR code detected
        }

        return data; // Return the decoded QR code data
    }

    std::optional<std::vector<cv::Point2f>> detectLine() {
        cv::Mat frame;
        if (cap_.read(frame)) {
            return std::nullopt;
        }

        if (frame.empty()) {
            return std::nullopt;
        }

        cv::Mat gray, threshold, edges;
        cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
        cv::threshold(gray, threshold, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);
        cv::Canny(threshold, edges, 50, 150);

        std::vector<cv::Point2f> points;

        int mid = edges.cols / 2;
        for (int y = edges.rows - 1; y >= edges.rows / 3; y -= 25) {
            int left = 0, right = edges.cols - 1;
            for (int x = mid - 1; x >= 0; --x) {
                if (edges.at<uchar>(y, x) > 128) {
                    left = x;
                    break;
                }
            }
            for (int x = mid; x < edges.cols; ++x) {
                if (edges.at<uchar>(y, x) > 128) {
                    right = x;
                    break;
                }
            }
            mid = (left + right) / 2;
            if (left < right) {
                points.emplace_back(mid, y);
            }
        }

        return points;
    }

    enum class TrafficLights {
        UNKNOWN = 0,
        RED,
        YELLOW,
        GREEN
    };

    TrafficLights detectTrafficLight() {
        cv::Mat frame;
        if (cap_.read(frame)) {
            return TrafficLights::UNKNOWN;
        }

        if (frame.empty()) {
            return TrafficLights::UNKNOWN;
        }

        cv::Mat hsv;
        cv::cvtColor(frame, hsv, cv::COLOR_BGR2HSV);

        // Define color ranges for red, yellow, and green
        const cv::Scalar lowerRed1(0, 100, 100), upperRed1(10, 255, 255);
        const cv::Scalar lowerYellow(20, 100, 100), upperYellow(30, 255, 255);
        const cv::Scalar lowerGreen(40, 100, 100), upperGreen(90, 255, 255);

        cv::Mat maskRed1, maskYellow, maskGreen;
        cv::inRange(hsv, lowerRed1, upperRed1, maskRed1);
        cv::inRange(hsv, lowerYellow, upperYellow, maskYellow);
        cv::inRange(hsv, lowerGreen, upperGreen, maskGreen);

        int redCount = cv::countNonZero(maskRed1);
        int yellowCount = cv::countNonZero(maskYellow);
        int greenCount = cv::countNonZero(maskGreen);

        if (redCount > yellowCount && redCount > greenCount) {
            return TrafficLights::RED;
        } else if (yellowCount > redCount && yellowCount > greenCount) {
            return TrafficLights::YELLOW;
        } else if (greenCount > redCount && greenCount > yellowCount) {
            return TrafficLights::GREEN;
        }

        return TrafficLights::UNKNOWN; // No clear traffic light detected
    }

private:
    cv::VideoCapture& cap_;
};
