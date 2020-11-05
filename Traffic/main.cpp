#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>

#include "TrafficLight.h"

//globals D:
std::mutex wait_mutex;
std::mutex idle_mutex;
std::condition_variable wait_cv;
std::condition_variable idle_cv;
bool wait_ready {};
bool idle_ready {};

namespace {
enum Evt {
    NO_EVENT = 0,
    WAIT_TIMER = 0x01,
    IDLE_TIMER = 0x02,
    TRAFFIC_CAR_NS_REQ = 0x04,
    TRAFFIC_CAR_EW_REQ = 0x08
};
int event = NO_EVENT;

std::mutex traffic_mutex;
std::condition_variable traffic_cv;

void setEvent(Evt evt) {
    {
        std::unique_lock<std::mutex> traffic_lock(traffic_mutex);
        event |= evt;
    }
    traffic_cv.notify_one();
}

}

void traffic_thread();
void input_thread();
void wait_timer();
void idle_timer();

int main() {
    std::thread(traffic_thread).detach();
    std::thread(input_thread).detach();
    std::thread(wait_timer).detach();
    std::thread(idle_timer).detach();

    while(true) {
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(60s);
    }
    return 0;
}

void traffic_thread() {
    TrafficLight traffic;

    while(true) {
        std::unique_lock<std::mutex> lk(traffic_mutex);
        traffic_cv.wait(lk, [] { return event != 0; });

        if(event & WAIT_TIMER) {
            traffic.waitTimer();
            event &= ~WAIT_TIMER;
        }
        if(event & IDLE_TIMER) {
            traffic.idleTimer();
            event &= ~IDLE_TIMER;
        }
        if(event & TRAFFIC_CAR_NS_REQ) {
            traffic.sendCarNsReq();
            event &= ~TRAFFIC_CAR_NS_REQ;
        }
        if(event & TRAFFIC_CAR_EW_REQ) {
            traffic.sendCarEwReq();
            event &= ~TRAFFIC_CAR_EW_REQ;
        }
    }
}

void input_thread() {
    uint32_t dir{};
    while(true) {
        std::cout << "Traffic direction (1: NS, 2: EW)? ";
        std::cin >> dir;

        switch(dir) {
        case 1:
            setEvent(TRAFFIC_CAR_NS_REQ);
            break;
        case 2:
            setEvent(TRAFFIC_CAR_EW_REQ);
            break;
        default:
            std::cout << std::endl << "---invalid option---" << std::endl;
        }
    }
}

void wait_timer() {
    using namespace std::chrono_literals;

    while(true) {
        std::unique_lock<std::mutex> lk(wait_mutex);
        wait_cv.wait(lk, []{ return wait_ready; });

        wait_ready = false;

        switch(TrafficLight::section) {
        case TrafficLight::Section::NSGo:
            std::this_thread::sleep_for(20000ms);
            break;
        case TrafficLight::Section::Slow:
            std::this_thread::sleep_for(3000ms);
            break;
        case TrafficLight::Section::EWGo:
            std::this_thread::sleep_for(10000ms);
            break;
        default:
            continue;
        }

        setEvent(WAIT_TIMER);
    }
}

void idle_timer() {
    using namespace std::chrono_literals;

    while(true) {
        std::unique_lock<std::mutex> lk(idle_mutex);
        idle_cv.wait(lk, []{ return idle_ready; });

        idle_ready = false;
        std::this_thread::sleep_for(15000ms);

        setEvent(IDLE_TIMER);
    }
}
