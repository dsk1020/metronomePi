#include <iostream>
#include <chrono>
#include <signal.h>
#include <thread>
#include <pigpio.h>
#include <vector>
#include <cmath>
#include <cpprest/json.h>

#include "rest.hpp"
#include "./metronome.hpp"

using namespace std::chrono_literals;
using namespace std;

#define LED_RED   17
#define LED_GREEN 27
#define BTN_MODE  24
#define BTN_TAP   23

volatile bool playMode = true;
volatile size_t blinkInterval = 0;
volatile bool run = true;
volatile int bpmCurrent = 0;
volatile int bpmMax = 0;
volatile int bpmMin = 0;

volatile sig_atomic_t signal_received = 0;
void sigint_handler(int signal) 
{
   signal_received = signal;
}

void getBPM(web::http::http_request msg) 
{
	msg.reply(web::http::status_codes::OK, web::json::value::number(bpmCurrent));

}
void postBPM(web::http::http_request msg) 
{
	msg.extract_json().then([msg](web::json::value body)
	{
		int temp = body.as_integer();
        if(temp < 0)
        {
           msg.reply(web::http::status_codes::BadRequest, body); 
        }
        else
        {
            bpmCurrent = temp;
            if(bpmMax == 0 && bpmMin == 0)
            {
                bpmMax = bpmCurrent;
                bpmMin = bpmCurrent;
            }
            else if(bpmCurrent > bpmMax)
            {
                bpmMax = bpmCurrent;
            }
            else if(bpmCurrent < bpmMin)
            {
                bpmMin = bpmCurrent;
            }
            blinkInterval = 60000 / bpmCurrent; // convert to miliseconds to flashing
		    msg.reply(web::http::status_codes::OK, body);
        }
	});
}

void getMIN(web::http::http_request msg) 
{
	msg.reply(web::http::status_codes::OK, web::json::value::number(bpmMin));
}
void delMIN(web::http::http_request msg) 
{
    bpmMin = 0;
	msg.reply(web::http::status_codes::OK);
}

void getMAX(web::http::http_request msg) 
{
	msg.reply(web::http::status_codes::OK, web::json::value::number(bpmMax));
}
void delMAX(web::http::http_request msg) 
{
    bpmMax = 0;
	msg.reply(web::http::status_codes::OK);
}

void play() {
	bool on = false;

	while (run) 
    {
        chrono::milliseconds ch = chrono::milliseconds(blinkInterval);
        this_thread::sleep_for(chrono::duration_cast<chrono::milliseconds>(ch));

		if (playMode)
        {
            on = !on;
            if(on && blinkInterval != 0)
            {
                gpioWrite(LED_GREEN, PI_HIGH);
            }
            else
            {
                gpioWrite(LED_GREEN, PI_LOW);
            }
        }
		else
        {
            on = false;
            gpioWrite(LED_GREEN, PI_LOW);
        }
	}
}

void learn(metronome& Metronome)
{
    bool debounce = false;
    while(run)
    {
        int btn = gpioRead(BTN_TAP);
        if(!playMode)
        {
            if(btn == PI_LOW && !debounce)
            {
                //std::cout << "PRESS TAP" << std::endl;
                Metronome.tap();
                debounce = true;
                gpioWrite(LED_RED, PI_HIGH);
            }
            else if(btn == PI_HIGH)
            {
                gpioWrite(LED_RED, PI_LOW);
                debounce = false;
            }
        }
        else
        {
            gpioWrite(LED_RED, PI_LOW);
        }
        
        std::this_thread::sleep_for(50ms);
    }
}

int main()
{
   if (gpioInitialise() == PI_INIT_FAILED) 
   {
      printf("ERROR: Failed to initialize the GPIO interface.\n");
      return 1;
   }

    gpioSetMode(LED_RED, PI_OUTPUT);
	gpioSetMode(LED_GREEN, PI_OUTPUT);
    gpioWrite(LED_GREEN, PI_LOW);

	gpioSetMode(BTN_MODE, PI_INPUT);
    gpioSetPullUpDown(BTN_MODE, PI_PUD_UP);

	gpioSetMode(BTN_TAP, PI_INPUT);
    gpioSetPullUpDown(BTN_TAP, PI_PUD_UP);

    auto bpm_rest = rest::make_endpoint("/bpm");
	bpm_rest.support(web::http::methods::GET, getBPM);
	bpm_rest.support(web::http::methods::POST, postBPM);

	auto min_rest = rest::make_endpoint("/bpm/min");
	min_rest.support(web::http::methods::GET, getMIN);
	min_rest.support(web::http::methods::DEL, delMIN);

	auto max_rest = rest::make_endpoint("/bpm/max");
	max_rest.support(web::http::methods::GET, getMAX);
	max_rest.support(web::http::methods::DEL, delMAX);

    bpm_rest.open().wait();
	min_rest.open().wait();
	max_rest.open().wait();

    metronome Metronome = metronome();

    thread playThread(play);
    thread learnThread(learn, ref(Metronome));
    playThread.detach();
    learnThread.detach();

    bool debounceMode = false;

    signal(SIGINT, sigint_handler);
    printf("Press CTRL-C to exit.\n");

    while(!signal_received)
    {
        int btn = gpioRead(BTN_MODE);

        if(btn == PI_LOW && !debounceMode)
        {
            //std::cout << "PRESS MODE" << std::endl;
            debounceMode = true;
            if(playMode)
            {
                Metronome.start_timing();
                playMode = false;
            }
            else
            {
                Metronome.stop_timing();

                size_t bpm = Metronome.get_bpm();
                if(bpm != 0)
                {
                    blinkInterval = bpm;
                    float sec = 60000.f / blinkInterval;
                    bpmCurrent = static_cast<int>(round(sec));

                    if(bpmCurrent > bpmMax)
                    {
                        bpmMax = bpmCurrent;
                    }
                    else if(bpmCurrent < bpmMin || bpmMin == 0)
                    {
                        bpmMin = bpmCurrent;
                    }
                    //cout << "BPM: " << bpmCurrent  << " blink: " << bpm<<" min:" << bpmMin << " max:" << bpmMax << endl;
                }

                playMode = true;
            }
        }
        else if(btn == PI_HIGH)
        {
            debounceMode = false;
        }
        std::this_thread::sleep_for(50ms);
    }

    run = false;
    gpioWrite(LED_RED, PI_LOW);
    gpioWrite(LED_GREEN, PI_LOW);
    gpioTerminate();
    bpm_rest.close();
    min_rest.close();
    max_rest.close();
    return 0;
}