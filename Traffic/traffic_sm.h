/*
 * traffic_sm.h
 *
 *  Created on: Oct 17, 2020
 *      Author: Victor
 */

#ifndef TRAFFIC_SM_H_
#define TRAFFIC_SM_H_

#include <iostream>
#include <boost/msm/back/state_machine.hpp>

#include <boost/msm/front/state_machine_def.hpp>
#include <boost/msm/front/functor_row.hpp>

namespace {
namespace msm = boost::msm;
namespace msmf = boost::msm::front;
namespace mpl = boost::mpl;
}

// ----- Events
struct Evt_WAIT_TIMER {
    Evt_WAIT_TIMER(){}
    template<class Event> Evt_WAIT_TIMER(Event const&){}
};
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

// ----- State machine

struct Traffic : public msmf::state_machine_def<Traffic> {
    // States
    struct Init : msmf::state<> {

    };
    struct Started : msmf::state_machine_def<Started> {
        template<class Event, class Fsm> void on_entry(Event const&, Fsm&) {
            std::cout << "Started::on_entry()" << std::endl;
        }

        struct Init : msmf::state<> {};
        struct NSGo : msmf::state_machine_def<NSGo> {
            //entry
            template<class Event, class Fsm> void on_entry(Event const&, Fsm&) {
                std::cout << "NSGo::on_entry()" << std::endl;
                //send internal event LAMP_GREEN_REQ for state machine LAMP_NS
                //send internal event LAMP_RED_REQ for state machine LAMP_EW
                //start sensing for EW cars
            }

            // States
            struct NSMinTimeWait: msmf::state<> {
                bool carWaiting;
                // Entry action
                template<class Event, class Fsm>
                void on_entry(Event const&, Fsm&) {
                    std::cout << "NSMinTimeWait::on_entry()" << std::endl;
                    carWaiting = false;
                    //start wait timer for 20 seconds
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
                //sm.process_event(LAMP_YELLOW_REQ(LAMP_NS))
            }
        };
        struct EWGo : msmf::state_machine_def<EWGo> {
            template<class Event, class Fsm> void on_entry(Event const&, Fsm&) {
                std::cout << "EWGo::on_entry()" << std::endl;
                //send internal event LAMP_GREEN_REQ for state machine LAMP_EW
                //send internal event LAMP_RED_REQ for state machine LAMP_NS
                //start idleTimer that runs for 15 seconds
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
                //sm.process_event(LAMP_YELLOW_REQ(LAMP_EW))
            }
        };

        using smNSGo = msm::back::state_machine<NSGo>;
        using smEWGo = msm::back::state_machine<EWGo>;

        using initial_state = smNSGo;

        // Transition table
        struct transition_table : mpl::vector<
            _row<smNSGo::exit_pt<NSGo::NSGoExit>, Evt_TRAFFIC_CAR_EW_REQ, NSSlow>,
            msmf::Row<NSSlow, Evt_WAIT_TIMER, smEWGo, msmf::none, msmf::none>,
            _row<smEWGo::exit_pt<EWGo::EWGoExit>, Evt_TRAFFIC_CAR_NS_REQ, EWSlow>,
            _row<smEWGo::exit_pt<EWGo::EWGoExit>, Evt_IDLE_TIMER, EWSlow>,
            msmf::Row<EWSlow, Evt_WAIT_TIMER, smNSGo, msmf::none, msmf::none>
        > {};
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

#endif /* TRAFFIC_SM_H_ */
