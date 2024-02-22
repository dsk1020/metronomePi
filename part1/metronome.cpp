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
    m_beat_count = 0;
    m_timing = true;
}
void metronome::stop_timing()
{
    m_timing = false;
}
void metronome::tap()
{
    if(m_timing)
    {
        auto now = chrono::high_resolution_clock::now();
        auto t = chrono::time_point_cast<chrono::milliseconds>(now);
        m_beats[m_index] = t.time_since_epoch().count();
        m_index = (m_index + 1) % beat_samples;
        m_beat_count ++;
    }
}
size_t metronome::get_bpm() const
{
    if(m_beat_count < beat_samples)
    {
        return 0;
    }

    size_t interval_1 = m_beats[((m_index + 1) % 4)] - m_beats[m_index];
    size_t interval_2 = m_beats[((m_index + 2) % 4)] - m_beats[((m_index + 1) % 4)];
    size_t interval_3 = m_beats[((m_index + 3) % 4)] - m_beats[((m_index + 2) % 4)];

    return ((interval_1 + interval_2 + interval_3) / 3);
}

