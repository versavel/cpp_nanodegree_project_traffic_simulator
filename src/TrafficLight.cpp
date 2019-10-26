#include <iostream>
#include <random>
#include <chrono>
#include <future>
#include "TrafficLight.h"

template <typename T>
T MessageQueue<T>::receive()
{
    // FP.5a : The method receive should use std::unique_lock<std::mutex> and _condition.wait() 
    // to wait for and receive new messages and pull them from the queue using move semantics. 
    // The received object should then be returned by the receive function. 

    // perform queue modification under the lock
    std::unique_lock<std::mutex> uLock(_mutex);

    // pass unique lock to condition variable
    _condition.wait(uLock, [this] { return !_queue.empty(); }); 

    // remove last message from queue
    T msg = std::move(_queue.back());
    _queue.pop_back();

    return msg; //not copied due to C++ return value optimization (RVO)
}

template <typename T>
void MessageQueue<T>::send(T &&msg)
{
    // FP.4a : The method send should use the mechanisms std::lock_guard<std::mutex> 
    // as well as _condition.notify_one() to add a new message to the queue and afterwards send a notification.
    
    // perform queue modification under the lock
    std::lock_guard<std::mutex> uLock(_mutex);

    // add message to queue
    std::cout << "Message " << msg << " sent to the queue" << std::endl;
    _queue.push_back(std::move(msg));
    
    // notify client after adding message to queue
    _condition.notify_one();
}

TrafficLight::TrafficLight()
{
    _currentPhase = TrafficLightPhase::red;
    _traffic_light_messages = std::make_shared<MessageQueue<TrafficLightPhase>>();
}

void TrafficLight::waitForGreen()
{
    // FP.5b : add the implementation of the method waitForGreen, in which an infinite while-loop 
    // runs and repeatedly calls the receive function on the message queue. 
    // Once it receives TrafficLightPhase::green, the method returns.

    while (true)
    {
        TrafficLightPhase phase = _traffic_light_messages->receive();
        if (phase == TrafficLightPhase::green) return;
    }
}

TrafficLightPhase TrafficLight::getCurrentPhase()
{
    std::lock_guard<std::mutex> ulock(_mtx);
    return _currentPhase;
}

void TrafficLight::simulate()
{
    // FP.2b : Finally, the private method „cycleThroughPhases“ should be started in a thread 
    // when the public method „simulate“ is called. To do this, use the thread queue in the base class. 
    
    // launch cycleThroughPhases function in a thread
    threads.emplace_back(std::thread(&TrafficLight::cycleThroughPhases, this));
}

// Construct a random number generator from a time-based seed with uniform distribution over the [4000, 6000) range
static unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
static std::default_random_engine generator (seed);
static std::uniform_int_distribution<int> distribution(4000,6000);

// private function which is executed in a thread
void TrafficLight::cycleThroughPhases()
{
    // FP.2a : Implement the function with an infinite loop that measures the time between two loop cycles 
    // and toggles the current phase of the traffic light between red and green and sends an update method 
    // to the message queue using move semantics. The cycle duration should be a random value between 4 and 6 seconds. 
    // Also, the while-loop should use std::this_thread::sleep_for to wait 1ms between two cycles. 
    
    while (true)
    {
        // wait for random amount of time 
        int duration = distribution(generator);
        std::this_thread::sleep_for(std::chrono::milliseconds(duration));
        
        // toggle traffic light under the lock
        std::lock_guard<std::mutex> ulock(_mtx);

        // toggle traffic light
        if (_currentPhase == TrafficLightPhase::red) 
            _currentPhase = TrafficLightPhase::green;
        else 
            _currentPhase = TrafficLightPhase::red;
        
        // send an update method to the message queue using move semantics
        auto ftr = std::async(std::launch::async, &MessageQueue<TrafficLightPhase>::send, _traffic_light_messages, std::move(_currentPhase));
		ftr.wait();
        
        // wait for 1 ms
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}