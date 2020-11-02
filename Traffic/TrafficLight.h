#ifndef TRAFFICLIGHT_H_
#define TRAFFICLIGHT_H_

#include <memory>
#include <atomic>

class TrafficLight {
public:
	TrafficLight();
	~TrafficLight();

	void sendCarNsReq();
	void sendCarEwReq();
	void waitTimer();
	void idleTimer();

	enum class Section {
		NSGo,
		Slow,
		EWGo
	};

	enum class Evt {
		WAIT_TIMER = 0,
		IDLE_TIMER,
		TRAFFIC_CAR_NS_REQ,
		TRAFFIC_CAR_EW_REQ
	};

	static std::atomic<Section> section;

private:
	struct Impl;
	std::unique_ptr<Impl> pImpl;
};

#endif /* TRAFFICLIGHT_H_ */
