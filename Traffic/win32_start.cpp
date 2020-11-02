
#include <iostream>
#include <windows.h>
#include <stdio.h>
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <thread>

#include "TrafficLight.h"

using namespace std::chrono_literals;

extern std::mutex wait_mutex;
extern std::mutex idle_mutex;
extern std::condition_variable wait_cv;
extern std::condition_variable idle_cv;
extern bool wait_ready;
extern bool idle_ready;

/**
 * event 0 -> TrafficLight::Evt::WAIT_TIMER
 * event 1 -> TrafficLight::Evt::IDLE_TIMER
 * event 2 -> TrafficLight::Evt::TRAFFIC_CAR_NS_REQ
 * event 3 -> TrafficLight::Evt::TRAFFIC_CAR_EW_REQ
 */
constexpr DWORD max_events {4};
HANDLE ghEvents[max_events];

DWORD WINAPI traffic_thread( LPVOID );
DWORD WINAPI input_thread( LPVOID );
DWORD WINAPI wait_timer( LPVOID );
DWORD WINAPI idle_timer( LPVOID );

void win32_start( void )
{
    HANDLE hThread;
    DWORD dwThreadID;

    for (DWORD i = 0; i < max_events; i++)
    {
        ghEvents[i] = CreateEvent(
            NULL,   // default security attributes
            FALSE,  // auto-reset event object
            FALSE,  // initial state is nonsignaled
            NULL);  // unnamed object

        if (ghEvents[i] == NULL)
        {
            printf("CreateEvent error: %d\n", GetLastError() );
            ExitProcess(0);
        }
    }

    // Create the traffic light thread
    hThread = CreateThread(
                 NULL,         // default security attributes
                 0,            // default stack size
                 (LPTHREAD_START_ROUTINE) traffic_thread,
                 NULL,         // no thread function arguments
                 0,            // default creation flags
                 &dwThreadID); // receive thread identifier

    if( hThread == NULL )
    {
        printf("CreateThread (traffic_thread) error: %d\n", GetLastError());
        ExitProcess(0);
    }

    // Create the wait timer thread
    hThread = CreateThread(
                 NULL,         // default security attributes
                 0,            // default stack size
                 (LPTHREAD_START_ROUTINE) wait_timer,
                 NULL,         // no thread function arguments
                 0,            // default creation flags
                 &dwThreadID); // receive thread identifier

    if( hThread == NULL )
    {
        printf("CreateThread (wait_timer) error: %d\n", GetLastError());
        ExitProcess(0);
    }

    // Create the idle timer thread
    hThread = CreateThread(
                 NULL,         // default security attributes
                 0,            // default stack size
                 (LPTHREAD_START_ROUTINE) idle_timer,
                 NULL,         // no thread function arguments
                 0,            // default creation flags
                 &dwThreadID); // receive thread identifier

    if( hThread == NULL )
    {
        printf("CreateThread (idle_timer) error: %d\n", GetLastError());
        ExitProcess(0);
    }

    // Create the input thread
    hThread = CreateThread(
                 NULL,         // default security attributes
                 0,            // default stack size
                 (LPTHREAD_START_ROUTINE) input_thread,
                 NULL,         // no thread function arguments
                 0,            // default creation flags
                 &dwThreadID); // receive thread identifier

    if( hThread == NULL )
    {
        printf("CreateThread (input_thread) error: %d\n", GetLastError());
        ExitProcess(0);
    }
    return;
}

DWORD WINAPI traffic_thread( LPVOID lpParam )
{
    // lpParam not used in this example
    UNREFERENCED_PARAMETER( lpParam);

	TrafficLight traffic;

	DWORD dwEvent;
    while(true) {
		// Wait for the thread to signal one of the event objects

		dwEvent = WaitForMultipleObjects(4,        	 // number of objects in array
										 ghEvents,   // array of objects
										 FALSE,      // wait for any object
										 INFINITE);

		switch (dwEvent) {
		// ghEvents[0] (TrafficLight::Evt::WAIT_TIMER) was signaled
		case WAIT_OBJECT_0 + 0:
			traffic.waitTimer();
			break;
			// ghEvents[1] (TrafficLight::Evt::IDLE_TIMER) was signaled
		case WAIT_OBJECT_0 + 1:
			traffic.idleTimer();
			break;
			// ghEvents[2] (TrafficLight::Evt::TRAFFIC_CAR_NS_REQ) was signaled
		case WAIT_OBJECT_0 + 2:
			traffic.sendCarNsReq();
			break;
			// ghEvents[3] (TrafficLight::Evt::TRAFFIC_CAR_EW_REQ) was signaled
		case WAIT_OBJECT_0 + 3:
			traffic.sendCarEwReq();
			break;
			// Return value is invalid.
		default:
			printf("Wait error: %d\n", GetLastError());
			ExitProcess(0);
		}
	}

    return 0;
}

DWORD WINAPI input_thread( LPVOID lpParam )
{
    // lpParam not used in this example
    UNREFERENCED_PARAMETER( lpParam);

	uint32_t dir{};
    while(true) {
		std::cout << "Traffic direction (1: NS, 2: EW)? ";
		std::cin >> dir;

		switch(dir) {
		case 1:
			if (!SetEvent(ghEvents[static_cast<int>(TrafficLight::Evt::TRAFFIC_CAR_NS_REQ)])) {
				printf("SetEvent failed (%d)\n", GetLastError());
			}
			break;
		case 2:
			if (!SetEvent(ghEvents[static_cast<int>(TrafficLight::Evt::TRAFFIC_CAR_EW_REQ)])) {
				printf("SetEvent failed (%d)\n", GetLastError());
			}
			break;
		default:
			std::cout << std::endl << "---invalid option---" << std::endl;
		}
    }

    return 0;
}

DWORD WINAPI wait_timer( LPVOID lpParam )
{
    // lpParam not used in this example
    UNREFERENCED_PARAMETER( lpParam);

    while(true) {
		std::unique_lock<std::mutex> lk(wait_mutex);
		wait_cv.wait(lk, []{ return wait_ready; });

		wait_ready = false;

		std::chrono::milliseconds milli;
		switch(TrafficLight::section) {
		case TrafficLight::Section::NSGo:
			milli = 20000ms;
			break;
		case TrafficLight::Section::Slow:
			milli = 3000ms;
			break;
		case TrafficLight::Section::EWGo:
			milli = 10000ms;
			break;
		}

		std::this_thread::sleep_for(milli);
		if (!SetEvent(ghEvents[static_cast<int>(TrafficLight::Evt::WAIT_TIMER)])) {
			printf("SetEvent failed (%d)\n", GetLastError());
		}
    }

    return 0;
}

DWORD WINAPI idle_timer( LPVOID lpParam )
{
    // lpParam not used in this example
    UNREFERENCED_PARAMETER( lpParam);

    while(true) {
		using namespace std::chrono_literals;
		// Wait for data
		std::unique_lock<std::mutex> lk(idle_mutex);
		idle_cv.wait(lk, []{ return idle_ready; });

		idle_ready = false;

		std::this_thread::sleep_for(15000ms);
		if (!SetEvent(ghEvents[static_cast<int>(TrafficLight::Evt::IDLE_TIMER)])) {
			printf("SetEvent failed (%d)\n", GetLastError());
		}
    }

    return 0;
}
