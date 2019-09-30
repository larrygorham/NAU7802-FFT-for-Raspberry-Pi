# NAU7802-FFT-for-Raspberry-Pi
This samples the NAU7802 load cell driver and FFT's the buffer for harmonic analysis purposes
The nau7802 is fixed at 320 Hz sampling and 128 amp gain
and I collect 1 second of data to compute the mean
then collect 10 seconds data with mean removed. The FFT,
actually DFT, is fast and accurate. Set here for 4096 to cover the 3200
sample buffer. I wanted C Code for analysis purposes 

