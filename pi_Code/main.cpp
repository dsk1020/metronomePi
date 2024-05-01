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

volatile bool playMode = true; // which mode is the player in
volatile size_t blinkInterval = 0; // each beats time interval in milliseconds
volatile bool run = true; // used to stop threads is user wants to quit

volatile int bpmCurrent = 0; // current bpm being displayed
volatile int bpmMax = 0; // highest bpm since last reset
volatile int bpmMin = 0; // lowest bpm since last reset

// Used to detect when user wants to quit, to complete proper shutdown procedure
volatile sig_atomic_t signal_received = 0;
void sigint_handler(int signal) 
{
   signal_received = signal;
}

//---------- API Endpoints ------------//
// BPM GET returns the active bpm
void getBPM(web::http::http_request msg) 
{
	web::json::value res;
    res[U("bpm")] = web::json::value::number(bpmCurrent);

    web::http::http_response response(web::http::status_codes::OK);
    response.headers().add(U("Access-Control-Allow-Origin"), U("*"));
    response.set_body(res);
    msg.reply(response);

}
// BPM POST takes in a number and sets it as the active bpm being displayed
void postBPM(web::http::http_request msg) 
{
    // Get the body of the JSON message
    
	msg.extract_json().then([=](web::json::value body) 
	{
        // convert the body to an integer
		int temp = body.as_integer();
        if(temp < 0) // if negative send back BadRequest:400 as this will overflow the size_t
        {
            web::http::http_response response(web::http::status_codes::BadRequest);
            response.headers().add(U("Access-Control-Allow-Origin"), U("*"));
            msg.reply(response);
        }
        else
        {
            bpmCurrent = temp; 
            // set the new bpm as either min,max or both
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
            blinkInterval = 60000 / bpmCurrent; // calculate the interval of each bpm to milliseconds for the play function
		    web::http::http_response response(web::http::status_codes::OK);
            response.headers().add(U("Access-Control-Allow-Origin"), U("*"));
            msg.reply(response);
        }
	});
}

// Return the current min bpm
void getMIN(web::http::http_request msg) 
{
    web::json::value res;
    res[U("bpm")] = web::json::value::number(bpmMin);

    web::http::http_response response(web::http::status_codes::OK);
    response.headers().add(U("Access-Control-Allow-Origin"), U("*"));
    response.set_body(res);
    msg.reply(response);
}
// reset the min bpm to 0
void delMIN(web::http::http_request msg) 
{
    bpmMin = 0;
    web::json::value res;
    res[U("bpm")] = web::json::value::number(bpmMin);

    web::http::http_response response(web::http::status_codes::OK);
    response.headers().add(U("Access-Control-Allow-Origin"), U("*"));
    response.set_body(res);
    msg.reply(response);
}

// Return the current max bpm
void getMAX(web::http::http_request msg) 
{
    web::json::value res;
    res[U("bpm")] = web::json::value::number(bpmMax);

	web::http::http_response response(web::http::status_codes::OK);
    response.headers().add(U("Access-Control-Allow-Origin"), U("*"));
    response.set_body(res);
    msg.reply(response);
}
// Reset the max bpm to 0 
void delMAX(web::http::http_request msg) 
{
    bpmMax = 0;
    web::json::value res;
    res[U("bpm")] = web::json::value::number(bpmMax);

	web::http::http_response response(web::http::status_codes::OK);
    response.headers().add(U("Access-Control-Allow-Origin"), U("*"));
    response.set_body(res);
    msg.reply(response);
}

//---------- PI Functionality ----------//
// used to play / blink the light at the set tempo
void play() {
	bool on = false;

	while (run) // while the user doesn't wish to terminate
    {
        chrono::milliseconds ch = chrono::milliseconds(blinkInterval); // convert the size_t to usable chrono miliseconds 
        this_thread::sleep_for(chrono::duration_cast<chrono::milliseconds>(ch)); // sleep for the interval

		if (playMode) // if light should blink
        {
            on = !on;
            if(on && blinkInterval != 0) // if the light should be on and there is an interval
            {
                gpioWrite(LED_GREEN, PI_HIGH); // turn on green led
            }
            else
            {
                gpioWrite(LED_GREEN, PI_LOW); // turn off green led
            }
        }
		else // if the status is in record mode
        {
            on = false;
            gpioWrite(LED_GREEN, PI_LOW); // keep the green light off
        }
	}
}

void learn(metronome& Metronome)
{
    bool debounce = false; // used to not allow holding down the button to count as many presses
    while(run)
    {
        int btn = gpioRead(BTN_TAP); // get the active state of the tap button
        if(!playMode)
        {
            if(btn == PI_LOW && !debounce) // if the button is pressed and it isn't being held down
            {
                Metronome.tap();  // record button press
                debounce = true; // limit button to not be held down
                gpioWrite(LED_RED, PI_HIGH); // turn on red led
            }
            else if(btn == PI_HIGH) // if button is released
            {
                gpioWrite(LED_RED, PI_LOW); // turn off red led
                debounce = false; // allow for another button press
            }
        }
        else
        {
            gpioWrite(LED_RED, PI_LOW); // keep red led off, this prevent a user form holding both buttons at the same time leaving the red on
        }
        
        std::this_thread::sleep_for(50ms); // sleep of the debounce time to not over poll the button
    }
}

int main()
{
    // initialize pigpio
   if (gpioInitialise() == PI_INIT_FAILED) 
   {
      printf("ERROR: Failed to initialize the GPIO interface.\n");
      return 1;
   }

    // set up led and buttons
    gpioSetMode(LED_RED, PI_OUTPUT);
	gpioSetMode(LED_GREEN, PI_OUTPUT);
    gpioWrite(LED_GREEN, PI_LOW);

	gpioSetMode(BTN_MODE, PI_INPUT);
    gpioSetPullUpDown(BTN_MODE, PI_PUD_UP);

	gpioSetMode(BTN_TAP, PI_INPUT);
    gpioSetPullUpDown(BTN_TAP, PI_PUD_UP);

    //---API---//
    auto bpm_rest = rest::make_endpoint("/bpm"); // create bpm endpoint
	bpm_rest.support(web::http::methods::GET, getBPM); // GET
	bpm_rest.support(web::http::methods::POST, postBPM); // POST

	auto min_rest = rest::make_endpoint("/bpm/min"); // create min endpoint
	min_rest.support(web::http::methods::GET, getMIN); // GET
	min_rest.support(web::http::methods::DEL, delMIN); // DELETE

	auto max_rest = rest::make_endpoint("/bpm/max"); // create max endpoint
	max_rest.support(web::http::methods::GET, getMAX); // GET
	max_rest.support(web::http::methods::DEL, delMAX); // DELETE

    // start api endpoints
    bpm_rest.open().wait();
	min_rest.open().wait();
	max_rest.open().wait();

    metronome Metronome = metronome(); // create instance of metronome

    // start threads for blinking the light and reading the taps
    thread playThread(play);
    thread learnThread(learn, ref(Metronome));
    playThread.detach();
    learnThread.detach();

    bool debounceMode = false; // used to limit to one key press

    signal(SIGINT, sigint_handler); // captures the event if the user want to quit
    printf("Press CTRL-C to exit.\n");

    while(!signal_received) // while not being terminated run
    {
        int btn = gpioRead(BTN_MODE);

        if(btn == PI_LOW && !debounceMode) // if the button is pressed down and isn't being held
        {
            debounceMode = true;
            if(playMode)
            {
                Metronome.start_timing(); // start record mode
                playMode = false; // switch to recording mode, allowing thread to run
            }
            else
            {
                Metronome.stop_timing(); // stop recording taps

                size_t bpm = Metronome.get_bpm(); // get the bpm
                if(bpm != 0) // if there wasn't an issue
                {
                    blinkInterval = bpm; // set the bpm to the blink time
                    float sec = 60000.f / blinkInterval; // calculate the bpm from milliseconds
                    bpmCurrent = static_cast<int>(round(sec)); // round to get clean number

                    // set min and max values
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
                }

                playMode = true; // play the blinking light
            }
        }
        else if(btn == PI_HIGH) // if the button is released allow it to be pressed again
        {
            debounceMode = false;
        }
        std::this_thread::sleep_for(50ms); // sleep for debounce time to limit polling amount
    }

    run = false; // used to terminate threads
    gpioWrite(LED_RED, PI_LOW); // turn off led's
    gpioWrite(LED_GREEN, PI_LOW);
    gpioTerminate(); 
    bpm_rest.close(); // close the api endpoints
    min_rest.close();
    max_rest.close();
    return 0;
}