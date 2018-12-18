#include <assert.h>
#include <string.h>
#include <iostream>
#include <cstdint>
#include <cmath>
#include <cuda_runtime.h>
#include <inc/cufft.h>
#include "conf.h"


class CUDASignalPreprocessor
{
private:
	int sample_rate;
	int raw_buffer_len;
	int frame_len;
	int frame_count;
	int in_frame_offset;
	int n_mel_filters;
	/*
		base_frec
		basal frecuencies from FFt bins basen don samplerate freq/nsamples
	*/
	float base_freq;
	float *signal_buffer;
	float *hamming_window;
	float **frames;
	/*
	float **power_frames
	 power spectrum (periodogram) using the following equation:
	*/
	float **power_frames;
	float **log_energy_frames;
	float *mfcc_frames;
	float *mel_values;
	float *freq_values;
	int n_mfcc_coefficients;

	kiss_fftr_cfg fft_cfg;

	
	/*
		void CUDASignalPreprocessor::applyPreEmphasis(void)

		S1 (n) = S(n) - alpha* S(n-1)
		Where  s  (n)  is  the  speech  signal  &  parameter  α  is  
		usually between  0.94  and  0.97. Pre-emphasis  is  needed  for 
		 high frequency in order to improve phone recognition performance.  
	*/
	void applyPreEmphasis(float);
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
	void framesFFTtoPowSpec(void);
	inline float melToHz(float);
	inline float hzToMel(float);
	float filterValue(int bank_index, float power_freq);
	void powerFramesToLogEnergies(void);
	void logEnergyToMFCC(void);

public:
	CUDASignalPreprocessor(float*,int,int,float,int);
	float* getFrame(int i);
	float* getPowerFrame(int i);
	int getFrameCount(void);
	int getMfccCount(void);
	void configureMFCC(int n_coefficients);
	void buildFilterBanks(int nfilters,int f0, int fmax);
	void getMfccCoefs(int* nframes,int* n_mfcpN_mfcc_coefficientsc_coefficients,float* output);
};