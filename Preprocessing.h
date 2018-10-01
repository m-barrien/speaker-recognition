#include <assert.h>
#include <string.h>
#include <iostream>
#include <cstdint>
#include <cmath>
#include "./include/kissfft/kiss_fftr.h"

class SignalPreprocessor
{
private:
	int raw_buffer_len;
	int frame_len;
	int frame_count;
	int in_frame_offset;
	float *signal_buffer;
	float *hamming_window;
	float **frames;
	/*
	float **power_frames
	 power spectrum (periodogram) using the following equation:
	*/
	float **power_frames;
	int16_t *out_signal_buffer;

	kiss_fftr_cfg fft_cfg;

public:
	SignalPreprocessor(float*,int,int,float);
	int getFrameCount(void);
	
	/*
		void SignalPreprocessor::applyPreEmphasis(void)

		S1 (n) = S(n) - alpha* S(n-1)
		Where  s  (n)  is  the  speech  signal  &  parameter  α  is  
		usually between  0.94  and  0.97. Pre-emphasis  is  needed  for 
		 high frequency in order to improve phone recognition performance.  
	*/
	void applyPreEmphasis(float);
	/*
		void dumpSignal(char*);

		Converts signal to 16Bit array in char pointer
	*/
	void dumpSignal(char*);
	/*
		Convert signal to frames
	*/
	void dumpToFrames(void);
	/*
		Applies hamming window to each frames
	*/
	void applyWindowsToFrames(void);
	/*
		Applies FFTs to frames

		void kiss_fftr(kiss_fftr_cfg cfg,const kiss_fft_scalar *timedata,kiss_fft_cpx *freqdata);

		input timedata has nfft scalar points
		output freqdata has nfft/2+1 complex points
	*/
	void framesFFT(void);
	float* getFrame(int i);
	float* getPowerFrame(int i);
};