# completed temperature&humidity reading, filtering, and PM conversion.

Parameters for conversion algorithm are copied from reference2. Should be generated from ML. 
The filtering algorithm and conversion are based on the average voltage from the 3 PM sensors. Using 3 separate voltages requires too much computation.

2-stage filtering: 1st stage - find the median from a batch (5 by default, can be changed) of input voltages
                            2nd stage - moving avg filtering, window size = 10 (can be changed)
*Note that temperature and humidity have also been filtered.

Connection with DHT11 temperature&humidity sensor:

GND (pin closest to "-" label) -> Arduino GND
VCC (middle pin) -> Arduino 5V
Signal (pin closest to "S" label) -> 5 (Digital)
