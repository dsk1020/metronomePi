#include <iostream>
#include <chrono>
#include <thread>
#include <pigpio.h>
#include <vector>

#include "./metronome.hpp"

using namespace std::chrono_literals;
using namespace std;

void metronome::start_timing()
{
    m_beat_count = 0; // rest the beat count
    m_timing = true; // allow timing
}
void metronome::stop_timing()
{
    m_timing = false; // disable timing
}
void metronome::tap()
{
    if(m_timing) // if timing allowed 
    {
        auto now = chrono::high_resolution_clock::now(); // using a high precision clock get the current time
        auto t = chrono::time_point_cast<chrono::milliseconds>(now); // convert to milliseconds
        m_beats[m_index] = t.time_since_epoch().count(); //convert the time in miliseconds to a format that can be stored as a size_t
        m_index = (m_index + 1) % beat_samples; // set the next index, if higher that beat_samples wrap around to the start and overwrite the data
        m_beat_count ++; // increate the beat count, used to determine if the min amount of beats reached
    }
}
size_t metronome::get_bpm() const
{
    if(m_beat_count < beat_samples) // if there are less beats than expected terminate
    {
        return 0;
    }

    // calculate the time interval between two presses. use mod to get the index in the circular array. 
    size_t interval_1 = m_beats[((m_index + 1) % 4)] - m_beats[m_index];
    size_t interval_2 = m_beats[((m_index + 2) % 4)] - m_beats[((m_index + 1) % 4)];
    size_t interval_3 = m_beats[((m_index + 3) % 4)] - m_beats[((m_index + 2) % 4)];

    return ((interval_1 + interval_2 + interval_3) / 3); // get the average in milliseconds and return it
}

