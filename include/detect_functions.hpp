#pragma once

#include <opencv2/opencv.hpp>
#include <optional>
#include <string>
#include <vector>

class Detector {
public:
  Detector(cv::VideoCapture &cap) : cap_(cap) {}

  std::optional<std::string> detectQRCode() {
    cv::Mat frame;
    if (!cap_.read(frame)) {
      return std::nullopt;
    }

    if (frame.empty()) {
      return std::nullopt;
    }

    cv::QRCodeDetector qrDecoder;
    std::string data;
    try {
      data = qrDecoder.detectAndDecode(frame);
    } catch (const std::exception &e) {
      return std::nullopt;
    }

    if (data.empty()) {
      return std::nullopt; // No QR code detected
    }

    return data; // Return the decoded QR code data
  }

  std::optional<std::vector<cv::Point2f>> detectLine() {
    cv::Mat frame;
    if (!cap_.read(frame)) {
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
        points.emplace_back(static_cast<float>(mid), static_cast<float>(y));
      }
    }

    return points;
  }

  enum class TrafficLights { UNKNOWN = 0, RED, YELLOW, GREEN };

  TrafficLights detectTrafficLight() {
    cv::Mat frame;
    if (!cap_.read(frame)) {
      return TrafficLights::UNKNOWN;
    }

    if (frame.empty()) {
      return TrafficLights::UNKNOWN;
    }

    cv::Mat hsv;
    cv::cvtColor(frame, hsv, cv::COLOR_BGR2HSV);

    // Define color ranges for red, yellow, and green
    const cv::Scalar lowerRed1(0, 189, 128), upperRed1(14, 255, 255);
    const cv::Scalar lowerYellow(23, 99, 144), upperYellow(57, 255, 255);
    const cv::Scalar lowerGreen(40, 141, 135), upperGreen(87, 255, 255);

    cv::Mat maskRed, maskYellow, maskGreen;
    cv::inRange(hsv, lowerRed1, upperRed1, maskRed);
    cv::inRange(hsv, lowerYellow, upperYellow, maskYellow);
    cv::inRange(hsv, lowerGreen, upperGreen, maskGreen);

    cv::Mat kernel =
        cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(9, 9));
    cv::morphologyEx(maskRed, maskRed, cv::MORPH_CLOSE, kernel);
    cv::morphologyEx(maskYellow, maskYellow, cv::MORPH_CLOSE, kernel);
    cv::morphologyEx(maskGreen, maskGreen, cv::MORPH_CLOSE, kernel);

    cv::Mat edgesRed, edgesYellow, edgesGreen;
    cv::Canny(maskRed, edgesRed, 25, 75);
    cv::Canny(maskYellow, edgesYellow, 25, 75);
    cv::Canny(maskGreen, edgesGreen, 25, 75);

    std::vector<std::vector<cv::Point>> contoursRed, contoursYellow,
        contoursGreen;
    cv::findContours(edgesRed, contoursRed, cv::RETR_CCOMP,
                     cv::CHAIN_APPROX_SIMPLE);
    cv::findContours(edgesYellow, contoursYellow, cv::RETR_CCOMP,
                     cv::CHAIN_APPROX_SIMPLE);
    cv::findContours(edgesGreen, contoursGreen, cv::RETR_CCOMP,
                     cv::CHAIN_APPROX_SIMPLE);

    std::vector<std::vector<cv::Point>> approxRed(contoursRed.size()),
        approxYellow(contoursYellow.size()), approxGreen(contoursGreen.size());
    for (size_t i = 0; i < contoursRed.size(); ++i) {
      cv::approxPolyDP(contoursRed[i], approxRed[i], 5, true);
    }
    for (size_t i = 0; i < contoursYellow.size(); ++i) {
      cv::approxPolyDP(contoursYellow[i], approxYellow[i], 5, true);
    }
    for (size_t i = 0; i < contoursGreen.size(); ++i) {
      cv::approxPolyDP(contoursGreen[i], approxGreen[i], 5, true);
    }

    auto max_red = std::max_element(
        approxRed.begin(), approxRed.end(),
        [](const std::vector<cv::Point> &a, const std::vector<cv::Point> &b) {
          return cv::contourArea(a) < cv::contourArea(b);
        });
    auto max_yellow = std::max_element(
        approxYellow.begin(), approxYellow.end(),
        [](const std::vector<cv::Point> &a, const std::vector<cv::Point> &b) {
          return cv::contourArea(a) < cv::contourArea(b);
        });
    auto max_green = std::max_element(
        approxGreen.begin(), approxGreen.end(),
        [](const std::vector<cv::Point> &a, const std::vector<cv::Point> &b) {
          return cv::contourArea(a) < cv::contourArea(b);
        });

    int redCount = (max_red != approxRed.end()) ? cv::contourArea(*max_red) : 0;
    int yellowCount =
        (max_yellow != approxYellow.end()) ? cv::contourArea(*max_yellow) : 0;
    int greenCount =
        (max_green != approxGreen.end()) ? cv::contourArea(*max_green) : 0;

    if (redCount < 50 && yellowCount < 50 && greenCount < 50) {
      return TrafficLights::UNKNOWN; // No significant traffic light detected
    }

    if (redCount > yellowCount && redCount > greenCount) {
      return TrafficLights::RED;
    }
    if (yellowCount > redCount && yellowCount > greenCount) {
      return TrafficLights::YELLOW;
    }
    if (greenCount > redCount && greenCount > yellowCount) {
      return TrafficLights::GREEN;
    }
    return TrafficLights::UNKNOWN; // No clear traffic light detected
  }

private:
  cv::VideoCapture &cap_;
};
