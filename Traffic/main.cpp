#include <thread>
#include <mutex>
#include <condition_variable>

#include "TrafficLight.h"

//globals D:
std::mutex wait_mutex;
std::mutex idle_mutex;
std::condition_variable wait_cv;
std::condition_variable idle_cv;
bool wait_ready {};
bool idle_ready {};

int main() {
#ifdef _WIN32
	extern void win32_start();
	win32_start();
#else
#endif
	while(true) {
		using namespace std::chrono_literals;
		std::this_thread::sleep_for(60s);
	}
	return 0;
}
