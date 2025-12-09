#include "Globals.h"
#include "Timer.h"

Timer::Timer() {

}

void Timer::start() {
	startPoint = std::chrono::steady_clock::now();
	runningTimer = true;
	accumulatedMS = 0;
}

long long Timer::read() {
	if (runningTimer) {
		std::chrono::steady_clock::time_point timeNow = std::chrono::steady_clock::now();
		accumulatedMS = accumulatedMS + std::chrono::duration_cast<std::chrono::milliseconds>(timeNow - startPoint).count();
		return accumulatedMS;
	}
	return accumulatedMS;
}

void Timer::stop() {
	if (runningTimer) {
		accumulatedMS = read();
		runningTimer = false;
	}
}