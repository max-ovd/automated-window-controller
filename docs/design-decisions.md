# Design Decisions

This document outlines the reasoning behind the major hardware decisions made during the development of the automated climate-controlled window. Each section considers alternative approaches, their trade-offs, and why a particular solution was selected.

## 1. LM7809 vs. Buck Converter

The system uses a 12V power supply to drive the linear actuator, while the Arduino and temperature sensor require a regulated 9V supply. This creates a need to step down the input voltage.

Two approaches were considered: a linear voltage regulator (LM7809) and a switching buck converter.

### LM7809 Linear Regulator

The LM7809 is a linear regulator that maintains a constant 9V output by dissipating the excess input voltage as heat.

Its power dissipation is approximately:

$$
P_{\text{dissipated}}=(V_{\text{in}}-V_{\text{out}})I
$$

For a 12V input and 9V output:

$$
P_{\text{dissipated}}=3I
$$

At a load current of 200mA (TODO: measure actual current using a multimeter), the regulator would dissipate approximately 0.6W as heat. This can require additional thermal management, particularly as current consumption increases.

For this project, the relatively low current requirements make a linear regulator a viable option, although actual power dissipation and operating temperature should be verified.

**Advantages:**
- Simple circuit with relatively few components.
- Low output voltage ripple and noise.
- Inexpensive and easy to implement.
- Compact implementation with relatively few components.

**Disadvantages:**
- Relatively inefficient when the input-output voltage difference is large.
- Excess power is dissipated as heat.
- May require a heatsink depending on current and operating conditions.

### Buck Converter

A buck converter uses high-frequency switching and energy-storage components to efficiently step down a DC voltage.

Unlike a linear regulator, it does not dissipate the entire voltage difference as heat. Instead, it transfers energy through an inductor and capacitor to produce a lower output voltage.

**Advantages:**
- Higher efficiency, particularly when stepping down from 12V to 9V.
- Less heat generation.
- Better suited for powering multiple electronic components.

**Disadvantages:**
- Switching introduces electrical noise and output ripple.
- More complex circuitry.
- Requires appropriate filtering and layout to minimize interference.

### Decision

A linear regulator was preferred for the 12V-to-9V supply because of the limited current draw from the Arduino and LM35DZ, as well as the relative simplicity of the circuit.

The absence of high-frequency switching noise was also a considerable advantage, particularly because the system uses an analog temperature sensor whose measurements can be affected by electrical interference.

---

## 2. LM35DZ vs. Other Temperature Sensors

Temperature measurement is the primary input to the window-control system. The sensor needed to provide sufficiently stable measurements for threshold-based control without introducing unnecessary hardware or software complexity.

Several types of temperature sensors were considered, including thermistors, digital temperature sensors, and the LM35DZ.

### LM35DZ

The LM35DZ is an analog temperature sensor with a linear output of approximately 10mV per degree Celsius.

Its output voltage is related to temperature by:

$$
V_{\text{out}}=0.01T
$$

where:
- $V_{\text{out}}$ is the output voltage in volts.
- $T$ is the temperature in degrees Celsius.

For example, an ambient temperature of 25°C produces an output of approximately 250mV.

The sensor can connect directly to an Arduino analog input, allowing temperature to be calculated using the ADC reading.

**Advantages:**
- Simple analog interface.
- Linear output simplifies temperature conversion.
- No communication protocol or additional libraries required.
- Low component cost and relatively low power consumption.

**Disadvantages:**
- Analog measurements are susceptible to electrical noise.
- ADC resolution and reference-voltage accuracy affect measurement precision.
- Requires appropriate signal conditioning for stable readings in electrically noisy environments.

### Thermistors

Thermistors are resistive temperature sensors whose resistance changes with temperature.

They are inexpensive and can provide good sensitivity, but their resistance-temperature relationship is nonlinear. Additional voltage-divider circuitry and mathematical conversion are needed to obtain accurate temperature measurements.

The potential accuracy advantage was deemed unnecessary for this project. Since the system uses relatively broad temperature thresholds for controlling window movement, the additional complexity of a thermistor-based measurement circuit was not considered worthwhile.

### Digital Temperature Sensors

Digital sensors provide temperature readings through a digital communication interface.

**Advantages:**
- Digital output reduces susceptibility to noise along the signal wire.
- Temperature conversion is handled internally.
- Can provide good measurement accuracy depending on the sensor.

**Disadvantages:**
- Requires a digital communication interface and additional software.
- May introduce more implementation complexity than necessary for a relatively simple control system.

### Decision

The LM35DZ was selected because its linear analog output provides a straightforward way to measure ambient temperature using the Arduino's built-in ADC.

The project does not require extremely high temperature accuracy or a complicated sensor interface. Instead, reliable measurements around the control thresholds are the priority.

To improve measurement stability, the firmware uses a circular buffer to calculate a moving average of multiple temperature readings. This helps reduce the influence of short-term fluctuations and random measurement noise.

---

## 3. RC Low-Pass Filters for Sensor Noise Reduction

The LM35DZ produces an analog voltage that is read by the Arduino's ADC. Because the sensor output is relatively small, electrical interference and fluctuations in the supply or ground can affect the measured temperature.

An RC low-pass filter is one potential method of reducing high-frequency noise before the signal reaches the ADC.

### How an RC Low-Pass Filter Works

An RC low-pass filter consists of a resistor and capacitor connected together.

A basic configuration places a resistor in series with the input signal and a capacitor between the output node and ground.

The capacitor's impedance depends on signal frequency:

$$
X_C=\frac{1}{2\pi fC}
$$

where:
- $X_C$ is capacitive reactance.
- $f$ is signal frequency.
- $C$ is capacitance.

At low frequencies, the capacitor has relatively high impedance, so most of the input signal appears at the output.

At high frequencies, the capacitor has lower impedance and provides a path for more of the noise signal to ground.

As a result, slowly changing signals such as ambient temperature measurements pass through relatively unchanged, while faster fluctuations are attenuated.

### Cutoff Frequency

The cutoff frequency determines the approximate boundary between frequencies that pass through the filter and frequencies that are increasingly attenuated.

$$
f_c=\frac{1}{2\pi RC}
$$

At the cutoff frequency, the output amplitude is approximately 70.7% of the input amplitude, corresponding to a reduction of 3dB.

For example, consider a filter using:

- $R=1\,k\Omega$
- $C=0.1\,\mu F$

The cutoff frequency is:

$$
f_c=\frac{1}{2\pi(1000)(0.1\times10^{-6})}
$$

$$
f_c\approx1591\,Hz
$$

This means that higher-frequency fluctuations are increasingly attenuated, while the relatively slow changes in ambient temperature can still be measured.

### Power Supply Decoupling and Sensor Filtering

Several RC filtering and decoupling configurations were implemented to mitigate electrical noise generated by the linear actuator and its motor driver.

**12V-to-9V regulator output to Arduino input:**

10µF and 0.1µF capacitors are connected in parallel between the LM7809 output and ground to reduce voltage fluctuations from propagating into the Arduino voltage supply

The two capacitors serve complementary purposes:

- **10µF:** Provides local bulk capacitance to help stabilize the supply against relatively slower voltage fluctuations.
- **0.1µF:** Provides high-frequency bypassing by presenting a relatively low impedance to high-frequency noise.

**Arduino-to-LM35DZ supply:**

Only the 0.1µF capacitor is used here to help filter out high-frequency noise.

**LM35DZ analog output:**

A 1k resistor in series with a 0.1µF capacitor to form a low-pass RC filter was used at the LM35 output to attenuate high-frequency noise before the signal reached the Arduino's ADC.

This filtering approach complements the software-based moving average implemented in the firmware, providing noise attenuation at both the hardware and software levels.

**Design takeaway:**

This implementation highlighted the importance of distinguishing between power-supply decoupling and signal filtering. Lots of learning was down on how to choose a cutoff frequency and a bit about different causes of noise in a circuit.



### Trade-offs

Reducing the cutoff frequency provides stronger attenuation of high-frequency noise, but also slows the response of the sensor.

For this application, given temperature measuremement intervals of a minute, this side effect is not paramount.

An excessively large RC time constant could delay the detection of temperature changes around the control thresholds.

### Application to This Project

An RC low-pass filter provides a hardware-level method of reducing high-frequency noise in the LM35DZ signal before it reaches the Arduino ADC.

It complements the software-based moving average filter:

- **Hardware filtering:** Attenuates high-frequency fluctuations before analog-to-digital conversion.
- **Software filtering:** Averages multiple ADC readings to reduce short-term measurement variation.

Using both approaches can improve measurement stability, provided the filter response is chosen carefully so that genuine temperature changes are not excessively delayed.


### Other

The Arduino Nano outputs the most stable 5V $\pm 0.05 \text{V}$ through its 5V input/output pin. As a result, we must power it through V<sub>in</sub> instead which is why we step the 12V supply down to 9V.

---

## Acknowledgement

Co-written with ChatGPT.