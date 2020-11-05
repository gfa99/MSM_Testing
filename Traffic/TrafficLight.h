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

	static std::atomic<Section> section;

private:
	struct Impl;
	std::unique_ptr<Impl> pImpl;
};

#endif /* TRAFFICLIGHT_H_ */
