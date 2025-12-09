#pragma once
#include "chrono";

class Timer {
public:
	Timer();

	void start();
	long long read();
	void stop();
private:
	std::chrono::steady_clock::time_point startPoint;

	bool runningTimer = false;
	long long accumulatedMS = 0;
};
