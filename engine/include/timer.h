#pragma once

#include <chrono>
#include <iostream>

class Timer {
public:
    Timer();
    void logDifference(const std::string &logText);

private:
    std::chrono::steady_clock::time_point start;
};