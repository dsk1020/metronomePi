# metronomePi
Simple digital metronome on the Raspberry Pi. Itt has 2 buttons with a "record" and a "play" functionality. See the current, man and min bpm on the dashboard and set the bpm from the dashboard.
# pi Wiring
## Red Led
- GPIO 17 is connected to a 220Ω resistor.
- he other end of the resistor is connected to the positive (+) of the Red LED.
- The negative (-) of the Red LED is connected to Ground (GND).
## Green Led
- GPIO 27 is connected to a 220Ω resistor.
- he other end of the resistor is connected to the positive (+) of the Green LED.
- The negative (-) of the Green LED is connected to Ground (GND).
## Mode Button
- 3.3V power is connected to a 10kΩ resistor.
-  The other end of the resistor is connected to one terminal of the Mode Button , the second terminal is connected to ground.
- The other sid of the Mode Button is connected to GPIO 24.
## Play Button
- 3.3V power is connected to a 10kΩ resistor.
-  The other end of the resistor is connected to one terminal of the Play Button , the second terminal is connected to ground.
- The other sid of the Play Button is connected to GPIO 23.
# How to Run
Verify pigpio is installed on the pi. Compile the pi code with the cmake file in /pi_Code, and run the dashboard with `npm install` then `npm run dev`.  
# React Endpoint API calls
- /bpm
  - GET: Empty JSON body
  - POST: Single positive integer in JSON Body
- /bpm/min
  - GET: Empty JSON body
  - DEL: Empty JSON body
- /bpm/max
  - GET: Empty JSON body
  - DEL: Empty JSON body