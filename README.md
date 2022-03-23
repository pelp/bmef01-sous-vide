# Sous vide

## Introduction
This project is ment to implement a sous vide using a microcotroller, a relay and some temperature sensors. The brains of this is an esp32. The esp32 is running a webserver that serve a few static files for the user interface and also has an API available where the interface can get data and set parameters.

## API
The API is realised as with a few GET and POST requests. Since the data returned from the microcontroller contains very little information and implementing a json based REST interface would be too much work, a simple plain text, space separated protocol is used instead.

## Relays
The relays are controlled with a simple pin change on the microcontroller, these relays are attached with a "shield" type board ontop of the esp32 development board.

## Sensors
To sense the temperature in the water a resistive temperature probe is used, this in combination with a pull-down resistor create a voltage divider which the microcontroller can sample. The most appropriate resistor was decided by trying to increase the dynamic range in the temperature range where the Sous vide will operate in. This temperature range is between 50-90 degrees.

Using this desmos calculation sheet one can change the $R_p$ resistor value to see how the "distance" between the points in the temperature range of interest increases and decreses. On the graph the X axis is the sensed ADC value from the micrcontroller (this max sense value can be changed with the $V_{in}$ parameter), the Y axis represents the output temperature.

## PID
The PID controller uses a simple PID loop tuned to give accurate temperature control in the operating range of the Sous vide. Because the heating element is controlled through a relay, the PID controller only turns on the heating element when the output value of the controller is above 50%. It also turns off the heating element if the output is below 50%.