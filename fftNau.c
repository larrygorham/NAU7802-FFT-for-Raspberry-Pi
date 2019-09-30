/*
 *   test and fft nau7802
sudo gcc fftNau.c -o fftNau -lwiringPi -lm -std=c99
  By Larry Gorham
The nau7802 is fixed at 320 Hz sampling and 128 gain
and I collect 1 second of data to compute the mean
then collect 10 seconds data with mean removed. The FFT,
actually DFT is fast and accurate. Set here for 4096.

*/

#include <wiringPiI2C.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <complex.h>

#define runRecords 3200
#define fftSize    4096
#define  DevAddr  0x2A  //device address

double PI;

typedef double complex cplx;

void delay(int);

void setBit(int fd, char registerAddress, char bitNumber)
{
  char value = wiringPiI2CReadReg8(fd, registerAddress);  //0xff - 
  value |= (0x01 << bitNumber); //Set this bit
  wiringPiI2CWriteReg8(fd, registerAddress, value);
}

void clearBit(int fd, char registerAddress, char bitNumber)
{
  char value = wiringPiI2CReadReg8(fd, registerAddress);
  value &= ~(1 << bitNumber); //Set this bit
  wiringPiI2CWriteReg8(fd, registerAddress, value);
}

char getBit(int fd, char registerAddress, char bitNumber)
{
  char value = wiringPiI2CReadReg8(fd, registerAddress);
  value &= (1 << bitNumber); //Clear all but this bit
  if (value > 0)
     return (0x01);
  else
     return (0x00);
}

void setLDO(int fd, char ldoValue)
{
  if (ldoValue > 7)
    ldoValue = 7; //Error check
  char value = wiringPiI2CReadReg8(fd, 0x01);
  value &= 0b11000111;    //Clear LDO bits
  value |= ldoValue << 3; //Mask in new LDO bits
  wiringPiI2CWriteReg8(fd, 0x01, value);
  setBit(fd, 0x00, 7); //Enable the internal LDO
}

void setGain(int fd, char gainValue)
{
  if (gainValue > 0b111)
    gainValue = 0b111; //Error check
 char value = wiringPiI2CReadReg8(fd, 0x01);
  value &= 0b11111000; //Clear gain bits
  value |= gainValue;  //Mask in new bits
  wiringPiI2CWriteReg8(fd, 0x01, value);
}

void setSampleRate(int fd, char rate)
{
  if (rate > 0b111)
    rate = 0b111; //Error check
  char value = wiringPiI2CReadReg8(fd, 0x02);
  value &= 0b10001111; //Clear CRS bits
  value |= rate << 4;  //Mask in new CRS bits
  wiringPiI2CWriteReg8(fd, 0x02, value);
}

int calibrateAFE(int fd)
{
  setBit(fd, 0x02, 2); //Begin calibration
  int count = 0;
  while (1)
  {
    if (getBit(fd, 0x02, 2) == 0)
      break; //Goes to 0 once cal is complete
    delay(1);
    count++;
    if (count > 10000)
      return (1);
  }
  return (count);
}


void readNau(cplx buf[], int samples)
{
	int runTime = 10; //seconds in run after average is removed
	int sampleRate = 320; // hz, set sample rate below or 80 
	int startAverageTime = 1; //seconds for average to remove bias

	FILE *temp_file;
	int i, fd, ctr, count, samplesToAverage;
	float startAverage;
	cplx valueMinus;
	time_t cur_time;
	unsigned long valueRaw;
	long valueShifted, valueFinal;
	char temp1, temp2, temp3;
	fd = wiringPiI2CSetup(DevAddr);

	// Is the device talking   ********************************
	if(-1 == fd)
		perror("I2C device setup error");	

	//Reset all registers  *************************************
	setBit(fd, 0x00, 0); //Set RR
	delay(1);
	clearBit(fd, 0x00, 0); //Clear RR to leave reset state

	delay(1);

	//Power on analog and digital sections of the scale *********
	setBit(fd, 0x00, 1);
	setBit(fd, 0x00, 2);

  	//Wait for Power Up bit to be set - takes approximately 200us
 	int counter = 0;
  	while (1)
	{
		if (getBit(fd, 0x00, 3) == 1) //1
			break; //Good to go
		delay(1);
		if (counter++ > 100)
		{	
			printf(" Error powering \n");
			break;
		}
 	 }

	delay(1);
	//Set LDO to 3.3V   
	setLDO(fd, 4);

	//Set gain to 128 
	setGain(fd,7); 

	//Set sample rate to 320 hz
	setSampleRate(fd, 7); 

	//Turn off CLK_CHP. From 9.1 power on sequencing.
	wiringPiI2CWriteReg8(fd, 0x15, 0x30); 

	//Enable 330pF decoupling cap on chan 2. From 9.14 application circuit note.
	setBit(fd, 0x1c, 7);

	//Calibrate analog front end of system. Returns 0 if CAL_ERR bit is 0 (no error)
	//Takes approximately 344ms to calibrate
	//Re-cal analog front end when we change gain, sample rate, or channel
	count = calibrateAFE(fd);
	if (count == 1)
		printf("Calibration Failed \n");
//    	else
//		printf("Loops to calibrate %d \n", count);




	temp_file =fopen("temp", "w");
	fclose(temp_file);  

	//Setup for first read
	setBit(fd, 0x00, 5);

	cur_time = time(NULL);	
//	printf("Start time %d \n", cur_time);
//	samples = (runTime + startAverageTime)*sampleRate;
	samplesToAverage = startAverageTime*sampleRate;
	startAverage = 0.;
	for(i=0; i<samples; i++)
	{
		while (1)
		{   
			if (getBit(fd, 0x00, 5) == 1) //1
      		break; //Good to read
 		   if (counter++ > 10000)
		   {	
				printf(" Error reading \n");
				break;
			}
		}

		temp1 = wiringPiI2CReadReg8(fd, 0x12);
		temp2 = wiringPiI2CReadReg8(fd, 0x13);
		temp3 = wiringPiI2CReadReg8(fd, 0x14);
		valueRaw = (unsigned long)temp1 << 16; //MSB
		valueRaw |= (unsigned long)temp2 << 8;          //MidSB
		valueRaw |= (unsigned long)temp3;               //LSB

	// the raw value coming from the ADC is a 24-bit number, so the sign bit now
	// resides on bit 23 (0 is LSB) of the uint32_t container. By shifting the
	// value to the left, I move the sign bit to the MSB of the uint32_t container.
	// By casting to a signed int32_t container I now have properly recovered
	// the sign of the original value
	valueShifted = (valueRaw << 8);
	// shift the number back right to recover its intended magnitude
	valueFinal = ( valueShifted >> 8 );

	counter = 0;
	if(i < samplesToAverage)
	{
		startAverage += (float)valueFinal/(float)samplesToAverage; 
		//printf("%d \n", valueFinal);
	}
	else // After computing the startAverage
	{	
//		temp_file =fopen("temp", "a");	
		valueMinus = 	(cplx)((double)valueFinal - startAverage + .5);
		buf[i] = valueMinus;
//		fprintf(temp_file,"%d \n", valueMinus);
//		fclose(temp_file);  
	}

	} //  End of i, samples loop

//	printf("Elasped time %d seconds.\n", time(NULL) - cur_time);
	return 0;
}

void _fft(cplx buf[], cplx out[], int n, int step)
{
	if (step < n) {
		_fft(out, buf, n, step * 2);
		_fft(out + step, buf + step, n, step * 2);
 
		for (int i = 0; i < n; i += 2 * step) {
			cplx t = cexp(-I * PI * i / n) * out[i + step];
			buf[i / 2]     = out[i] + t;
			buf[(i + n)/2] = out[i] - t;
		}
	}
}
 
void fft(cplx buf[], int n)
{
	cplx out[n];
	for (int i = 0; i < n; i++) out[i] = buf[i];
 
	_fft(buf, out, n, 1);
}
 
int main(void)
{
	PI = atan2(1, 1) * 4;
	cplx buf[fftSize] ;

	double freq, temp1, temp2, temp3;
	int i;
	int sampleRate = 320; //Hz
	int printSize = 2000; //bins to print centered on freq

    printf("Please input frequency: ");
    scanf("%lf", &freq);
	for(i=0; i<fftSize; i++)
	{
		buf[i] = 0.;
	}
	
	readNau(buf, runRecords);	

	fft(buf, fftSize);
	
	temp3 = (int)(freq*(fftSize-2)/(double)sampleRate + .5);
	for(i=temp3-printSize/2+1; i<temp3+printSize/2+1; i++)
	{
	temp1 = i*sampleRate/(double)(fftSize-2);
	temp2 = 2.*sqrt(creal(buf[i])*creal(buf[i])+cimag(buf[i])*cimag(buf[i]))/runRecords;
	if(temp2 > 90.)
		printf("%d %7.2f %5.0f\n", i, temp1, temp2);
	}

}
