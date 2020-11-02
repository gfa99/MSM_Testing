/*
 * TrafficLight.cpp
 *
 *  Created on: Oct 24, 2020
 *      Author: Victor
 */

#include "TrafficLight.h"



#include <iostream>
#include <boost/msm/back/state_machine.hpp>

#include <boost/msm/front/state_machine_def.hpp>
#include <boost/msm/front/functor_row.hpp>

#include <boost/shared_ptr.hpp>

#include <future>
#include <chrono>
#include <thread>
#include <functional>
#include <mutex>
#include <condition_variable>

extern std::mutex wait_mutex;
extern std::mutex idle_mutex;
extern std::condition_variable wait_cv;
extern std::condition_variable idle_cv;
extern bool wait_ready;
extern bool idle_ready;

namespace {
namespace msm = boost::msm;
namespace msmf = boost::msm::front;
namespace mpl = boost::mpl;

// ----- Events
struct Evt_WAIT_TIMER {};
struct Evt_IDLE_TIMER {
    Evt_IDLE_TIMER(){}
    template<class Event> Evt_IDLE_TIMER(Event const&){}
};
struct Evt_TRAFFIC_CAR_NS_REQ {
    Evt_TRAFFIC_CAR_NS_REQ(){}
    template<class Event> Evt_TRAFFIC_CAR_NS_REQ(Event const&){}
};
struct Evt_TRAFFIC_CAR_EW_REQ {
    Evt_TRAFFIC_CAR_EW_REQ(){}
    template<class Event> Evt_TRAFFIC_CAR_EW_REQ(Event const&){}
};
struct Evt_LAMP_GREEN_REQ_NS {};
struct Evt_LAMP_GREEN_REQ_EW {};
struct Evt_LAMP_YELLOW_REQ_NS {};
struct Evt_LAMP_YELLOW_REQ_EW {};
struct Evt_LAMP_RED_REQ_NS {};
struct Evt_LAMP_RED_REQ_EW {};

// ----- State machine

struct Traffic : public msmf::state_machine_def<Traffic> {
    // States
    struct Init : msmf::state<> {

    };
    struct Started : msmf::state_machine_def<Started> {
        template<class Event, class Fsm> void on_entry(Event const&, Fsm&) {
            std::cout << "Started::on_entry()" << std::endl;
        }

        //orthogonal region 0
        struct NSGo : msmf::state_machine_def<NSGo> {
            //entry
            template<class Event, class Fsm> void on_entry(Event const&, Fsm& sm) {
                std::cout << "NSGo::on_entry()" << std::endl;
                sm.process_event(Evt_LAMP_RED_REQ_EW());
                sm.process_event(Evt_LAMP_GREEN_REQ_NS());
                //start sensing for EW cars
            }

            // States
            struct NSMinTimeWait: msmf::state<> {
                bool carWaiting;
                // Entry action
                template<class Event, class Fsm>
                void on_entry(Event const&, Fsm& sm) {
                	using namespace std::literals;
                    std::cout << "NSMinTimeWait::on_entry()" << std::endl;
                    carWaiting = false;
                    //start wait timer for 20 seconds
                    {
                    	std::lock_guard<std::mutex> lk(wait_mutex);
                    	wait_ready = true;
                    	TrafficLight::section.store(TrafficLight::Section::NSGo);
                    }
                    wait_cv.notify_one();
                }
                // Exit action
                template<class Event, class Fsm>
                void on_exit(Event const&, Fsm& sm) {
                    if(carWaiting) {
                        //send TRAFFIC_CAR_EW_REQ event
                        sm.process_event(Evt_TRAFFIC_CAR_EW_REQ());
                    }
                }
            };

            struct NSMinTimeExceeded : msmf::state<> {
                template<class Event, class Fsm>
                void on_entry(Event const&, Fsm&) {
                    std::cout << "NSMinTimeExceeded::on_entry()" << std::endl;
                }
            };

            struct NSGoExit : public msmf::exit_pseudo_state<Evt_TRAFFIC_CAR_EW_REQ> {};

            // Action
            struct EWCarApproach {
                template<class Event, class Fsm, class SourceState, class TargetState>
                void operator()(Event const&, Fsm &, SourceState & src, TargetState &) const
                {
                    src.carWaiting = true;
                }
            };

            // Set initial state
            using initial_state = NSMinTimeWait;

            // Transition table
            struct transition_table: mpl::vector<
                    //        Start   Event         Next    Action      Guard
                    //       +-------+-------------+-------+-----------+------------
                    msmf::Row<NSMinTimeWait, Evt_WAIT_TIMER, NSMinTimeExceeded, msmf::none, msmf::none>,
                    msmf::Row<NSMinTimeWait, Evt_TRAFFIC_CAR_EW_REQ, msmf::none, EWCarApproach, msmf::none>,
                    msmf::Row<NSMinTimeExceeded, Evt_TRAFFIC_CAR_EW_REQ, NSGoExit, msmf::none, msmf::none>
            > {};
        };
        struct NSSlow : msmf::state<> {
            template<class Event, class Fsm> void on_entry(Event const&, Fsm& sm) {
                std::cout << "NSSlow::on_entry()" << std::endl;
                //start wait timer for 3 seconds
                sm.process_event(Evt_LAMP_YELLOW_REQ_NS());
                {
                	std::lock_guard<std::mutex> lk(wait_mutex);
                	wait_ready = true;
                	TrafficLight::section.store(TrafficLight::Section::Slow);
                }
                wait_cv.notify_one();
            }
        };
        struct EWGo : msmf::state_machine_def<EWGo> {

            template<class Event, class Fsm> void on_entry(Event const&, Fsm& sm) {
                std::cout << "EWGo::on_entry()" << std::endl;
                sm.process_event(Evt_LAMP_RED_REQ_NS());
                sm.process_event(Evt_LAMP_GREEN_REQ_EW());
                //start idleTimer that runs for 15 seconds
                {
                	std::lock_guard<std::mutex> lk(idle_mutex);
                	idle_ready = true;
                }
                idle_cv.notify_one();
                //start sensing for EW cars
            }

            // States
            struct EWMinTimeWait : msmf::state<> {
                bool carWaiting;
                //entry
                template<class Event, class Fsm> void on_entry(Event const&, Fsm&) {
                    std::cout << "EWMinTimeWait::on_entry()" << std::endl;
                    carWaiting = false;
                    //start waitTimer that runs for 10 seconds
                    {
                    	std::lock_guard<std::mutex> lk(wait_mutex);
                    	wait_ready = true;
                    	TrafficLight::section.store(TrafficLight::Section::EWGo);
                    }
                    wait_cv.notify_one();
                }
                //exit
                template<class Event, class Fsm> void on_exit(Event const&, Fsm& sm) {
                    if(carWaiting) {
                        sm.process_event(Evt_TRAFFIC_CAR_NS_REQ());
                    }
                }
            };

            struct EWMinTimeExceeded : msmf::state<> {
                template<class Event, class Fsm> void on_entry(Event const&, Fsm&) {
                    std::cout << "EWMinTimeExceeded::on_entry()" << std::endl;
                }
            };

            struct EWGoExit : public msmf::exit_pseudo_state<Evt_TRAFFIC_CAR_NS_REQ> {};

            // Actions
            struct NSCarApproach {
                template<class Event, class Fsm, class SourceState, class TargetState>
                void operator()(Event const&, Fsm &, SourceState & src, TargetState &) const
                {
                    src.carWaiting = true;
                }
            };
            struct EWCarApproach {
                template<class Event, class Fsm, class SourceState, class TargetState>
                void operator()(Event const&, Fsm &, SourceState &, TargetState &) const
                {
                    //restart the idleTimer
                    std::cout << "restart idleTimer" << std::endl;
                }
            };

            using initial_state = EWMinTimeWait;

            // Transition table
            struct transition_table : mpl::vector<
                msmf::Row<EWMinTimeWait, Evt_WAIT_TIMER, EWMinTimeExceeded, msmf::none, msmf::none>,
                msmf::Row<EWMinTimeWait, Evt_TRAFFIC_CAR_NS_REQ, msmf::none, NSCarApproach, msmf::none>,
                msmf::Row<EWMinTimeWait, Evt_TRAFFIC_CAR_EW_REQ, msmf::none, EWCarApproach, msmf::none>,
                msmf::Row<EWMinTimeExceeded, Evt_TRAFFIC_CAR_NS_REQ, EWGoExit, msmf::none, msmf::none>,
                msmf::Row<EWMinTimeExceeded, Evt_TRAFFIC_CAR_EW_REQ, msmf::none, EWCarApproach, msmf::none>,
                msmf::Row<EWMinTimeExceeded, Evt_IDLE_TIMER, EWGoExit, msmf::none, msmf::none>
            > {};
        };
        struct EWSlow : msmf::state<> {
            //entry
            template<class Event, class Fsm> void on_entry(Event const&, Fsm& sm) {
                std::cout << "EWSlow::on_entry()" << std::endl;
                //start wait timer for 3 seconds
                sm.process_event(Evt_LAMP_YELLOW_REQ_EW());
                {
                	std::lock_guard<std::mutex> lk(wait_mutex);
                	wait_ready = true;
                	TrafficLight::section.store(TrafficLight::Section::Slow);
                }
                wait_cv.notify_one();
            }
        };

        //orthogonal region 1 & 2 base class
        struct LAMP_NS : msmf::state_machine_def<LAMP_NS> {
        	static constexpr auto print = [](const std::string& color) {
        		std::cout << "LAMP_NS::" << color << std::endl;
        	};

        	struct Red : msmf::state<> {
        		template<class Event, class Fsm> void on_entry(Event const&, Fsm& sm) {
        			print("RED");
        		}
        	};
        	struct Green : msmf::state<> {
        		template<class Event, class Fsm> void on_entry(Event const&, Fsm& sm) {
        			print("GREEN");
        		}
        	};
        	struct Yellow : msmf::state<> {
        		template<class Event, class Fsm> void on_entry(Event const&, Fsm& sm) {
        			print("YELLOW");
        		}
        	};

        	using initial_state = Red;

        	// Transition table
        	struct transition_table : mpl::vector<
			_row<Red, Evt_LAMP_GREEN_REQ_NS, Green>,
			_row<Green, Evt_LAMP_YELLOW_REQ_NS, Yellow>,
			_row<Yellow, Evt_LAMP_RED_REQ_NS, Red>
			> {};
        };
        struct LAMP_EW : msmf::state_machine_def<LAMP_EW> {
        	static constexpr auto print = [](const std::string& color) {
        		std::cout << "LAMP_EW::" << color << std::endl;
        	};

        	struct Red : msmf::state<> {
        		template<class Event, class Fsm> void on_entry(Event const&, Fsm& sm) {
        			print("RED");
        		}
        	};
        	struct Green : msmf::state<> {
        		template<class Event, class Fsm> void on_entry(Event const&, Fsm& sm) {
        			print("GREEN");
        		}
        	};
        	struct Yellow : msmf::state<> {
        		template<class Event, class Fsm> void on_entry(Event const&, Fsm& sm) {
        			print("YELLOW");
        		}
        	};

        	using initial_state = Red;

        	// Transition table
        	struct transition_table : mpl::vector<
			_row<Red, Evt_LAMP_GREEN_REQ_EW, Green>,
			_row<Green, Evt_LAMP_YELLOW_REQ_EW, Yellow>,
			_row<Yellow, Evt_LAMP_RED_REQ_EW, Red>
			> {};
        };

        using smNSGo = msm::back::state_machine<NSGo>;
        using smEWGo = msm::back::state_machine<EWGo>;
        using lampNS = msm::back::state_machine<LAMP_NS>;
        using lampEW = msm::back::state_machine<LAMP_EW>;

        using initial_state = mpl::vector<smNSGo, lampNS, lampEW>;

        // Transition table
        struct transition_table : mpl::vector<
            _row<smNSGo::exit_pt<NSGo::NSGoExit>, Evt_TRAFFIC_CAR_EW_REQ, NSSlow>,
            msmf::Row<NSSlow, Evt_WAIT_TIMER, smEWGo, msmf::none, msmf::none>,
            _row<smEWGo::exit_pt<EWGo::EWGoExit>, Evt_TRAFFIC_CAR_NS_REQ, EWSlow>,
            _row<smEWGo::exit_pt<EWGo::EWGoExit>, Evt_IDLE_TIMER, EWSlow>,
            msmf::Row<EWSlow, Evt_WAIT_TIMER, smNSGo, msmf::none, msmf::none>
        > {};

        // Replaces the default no-transition response.
        template <class FSM,class Event>
        void no_transition(Event const& e, FSM&,int state)
        {
            std::cout << "no transition from state " << state
                << " on event " << typeid(e).name() << std::endl;
        }
    };

    using smStarted = msm::back::state_machine<Started>;

    using initial_state = smStarted;

    // Transition table
    struct transition_table : mpl::vector<
        //no transitions
    > {};

    // Replaces the default no-transition response.
    template <class FSM,class Event>
    void no_transition(Event const& e, FSM&,int state)
    {
        std::cout << "no transition from state " << state
            << " on event " << typeid(e).name() << std::endl;
    }
};

using smTraffic = msm::back::state_machine<Traffic>;

} // namespace

struct TrafficLight::Impl
{
	boost::shared_ptr<void> sm;

	Impl()
	: sm(new smTraffic)
	{}
};

std::atomic<TrafficLight::Section> TrafficLight::section = Section::NSGo;

TrafficLight::TrafficLight()
: pImpl(std::make_unique<Impl>())
{
	boost::static_pointer_cast<smTraffic>(pImpl->sm)->start();
}

TrafficLight::~TrafficLight() = default;

void TrafficLight::sendCarNsReq()
{
	boost::static_pointer_cast<smTraffic>(pImpl->sm)->process_event(Evt_TRAFFIC_CAR_NS_REQ());
}

void TrafficLight::sendCarEwReq()
{
	boost::static_pointer_cast<smTraffic>(pImpl->sm)->process_event(Evt_TRAFFIC_CAR_EW_REQ());
}

void TrafficLight::waitTimer()
{
	boost::static_pointer_cast<smTraffic>(pImpl->sm)->process_event(Evt_WAIT_TIMER());
}

void TrafficLight::idleTimer()
{
	boost::static_pointer_cast<smTraffic>(pImpl->sm)->process_event(Evt_IDLE_TIMER());
}
