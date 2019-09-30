# NAU7802-FFT-for-Raspberry-Pi
This samples the NAU7802 load cell driver module and FFT's the buffer for harmonic analysis purposes.
This entire program with nau7802 subroutine is stand alone requiring only standard libs. 
The NAU is fixed at the max, 320 Hz sampling and 128 amp gain but that's easily changed
to your tastes. Just change the control register bit loading. It's also fixed to collect
1 second of data and compute the mean, then collect 10 seconds data with mean removed. 

The FFT, actually DFT, is flexable, fast and accurate. It does require the C99 compiler on
the Raspberry. Just compile with 

sudo gcc fftNau.c -o fftNau -lwiringPi -lm -std=c99

then execute with sudo ./fftNau

and off you go. Here it's set for 4096 to cover the 3200
sample buffer. I output the larger amplitudes to identify harmonics.  The device is unshielded so lots of erroneous
60 Hz shows up. 

I wanted stand alone C Code for portability and analysis purposes. Please excuse my very rough coding. 
It does accomplish the job very well.

