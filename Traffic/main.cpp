/*
 * main.cpp
 *
 *  Created on: Oct 16, 2020
 *      Author: Victor
 */
#include <iostream>
#include <unistd.h>
#include "traffic_sm.h"

int main() {

    smTraffic traffic;

    traffic.start();
    traffic.process_event(Evt_TRAFFIC_CAR_EW_REQ());
    sleep(2);
    traffic.process_event(Evt_WAIT_TIMER());
    sleep(1);
    traffic.process_event(Evt_WAIT_TIMER());
    sleep(1);
    traffic.process_event(Evt_TRAFFIC_CAR_NS_REQ());
    traffic.process_event(Evt_WAIT_TIMER());
    sleep(1);
    traffic.process_event(Evt_WAIT_TIMER());


    sleep(1);
    traffic.process_event(Evt_WAIT_TIMER());
    sleep(3);
    traffic.process_event(Evt_TRAFFIC_CAR_EW_REQ());
    sleep(1);
    traffic.process_event(Evt_WAIT_TIMER());
    sleep(1);
    traffic.process_event(Evt_WAIT_TIMER());
    sleep(1);
    traffic.process_event(Evt_IDLE_TIMER());
    sleep(1);
    traffic.process_event(Evt_WAIT_TIMER());
	return 0;
}
