# OCXO Controller 

*Created by @dabecart, 2025*

## Notes about the timer's configuration

### TIM1: Receives the OCXO signal and replicates it internally

- Slave mode: Trigger Mode
- Trigger Source: TI2FP2
- Clock source: ETR (OCXO)

Receives the output of the OCXO on its ETR pin (PC4). The OCXO's output is therefore used in TIM1 as its clock source. TIM1 has a counter period of 1, meaning it resets/updates at the same time as the OCXO signal (10 MHz). The trigger output (TRGO) raises a trigger on every update meaning that all timers which use ITR0 (the TRGO of TIM1) and are set as Slave Mode = "External Clock Mode 1", will run at the same frequency as the OCXO.

TIM1 is set as Slave mode: "Trigger Mode" on "TI2FP2" (PA9) which is connected to PPS_REF, the reference signal for the calibration of the OCXO. This is used to start TIM1 at the same time as the PPS_REF. 

Channel 3 is set as "Output Compare" with Pulse = 0 and Mode = "Toggle on match". It will replicate the OCXO signal but at a 3V3 level.

**NOTE.** By setting the counter period to 1, the clock signal is being divided by 2, that is, 5MHz is being put into the ITR0, not 10! The counter period cannot be 0, the TIM doesn't turn on with this value...

### TIM5: Divides the OCXO signal down to the reference PPS frequency

- Slave mode: External Clock Mode 1
- Trigger source: ITR0
- Clock source: (TIM1 -> OCXO)

This timer was used as a hack to not route the OCXO_OUT signal to multiple pins, but also it is in charge of dividing the OCXO frequency down to the frequency of the reference PPS. This is accomplished by setting the "Prescaler" to 999 and the "Counter period" to 4999. This converts a 10 MHz signal to 1 Hz. 

Channel 2 is set as "Output Compare" (PA1). On this pin the divided OCXO signal will be generated.

TIM5 generates a TRGO (Trigger Output) on the Output Compare 2. This is used to start at the same time the timestamping clocks (TIM2 and TIM15).

**NOTE.** As previously said, ITR0 is 5MHz not 10MHz, therefore the Counter Period must be 2499. 

### TIM2: Timestamps the OCXO output divided down to the PPS reference frequency

- Slave mode: Trigger Mode
- Trigger Source: ITR4
- Clock source: Internal

TIM2 is used to get the timestamps of the OCXO output after being divided to the reference PPS' frequency. These timestamps will be compared to the timestamps of the reference PPS to tune the OCXO. 

The shrinked down OCXO signal is being replicated by TIM5 at pin PA1. Channel 1 (PA0) and Channel 3 (PA2) are set as "Input Capture direct mode". They will timestamp the rising and falling edge of the divided OCXO signal.

Note: TIM2 is a 32 bit timer. As timestamps are being made in TIM2 and TIM15, they should have the same range. That is why TIM2 has its "Counter Period" set to 65535, to match that of TIM15.

TIM2 is set as Slave Mode "Trigger Mode" with Trigger Source "ITR4" so that this timer starts working on the first pulse of the divided OCXO signal.

### TIM15: Timestamps the PPS reference

- Slave mode: Trigger Mode
- Trigger Source: ITR4
- Clock source: Internal

TIM15 is used to timestamp with the clock of the microcontroller the rising and falling edges of the PPS_REF signal. Channel 1 (PB15) and 2 (PB14) are set as "Input Capture direct mode".

TIM15 is set as Slave Mode "Trigger Mode" with Trigger Source "ITR4" so that this timer starts working on the the first pulse of the divided OCXO signal.

### TIM3, TIM4, TIM8: Generate the OUT1, OUT2 and OUT3 signals

- Slave mode: External Clock Mode 1
- Trigger Source: ITR0
- Clock source: (TIM1 -> OCXO)

These timers each have a single channel set as "PWM Generation". They are used to generate PWM outputs. The "Counter Period" can be used in combination with the "Prescaler" to set the frequency of the PWM. The "Pulse" of the PWM controls the duty cycle. To set the phase of the signals, the counter of the TIMx can be set initially to a specific value. The only thing about the phase is that during this the ITR0 must be deactivated (TIM1 must not generate a signal). 