#include "CUDAPreprocessing.h"

__constant__ wave_params_t dev_wave_params;

__device__ __forceinline__ float  hamming_window(int j){
	return ( 0.54 - 0.46 * cos((2*M_PI*j)/(dev_wave_params.frame_len-1)) );
}
__device__ void dev_applyPreEmphasis(float emph){

}
__device__ void dev_dumpToFrames(){

}
__device__ void dev_applyWindowsToFrames(){

}
__device__ void dev_framesFFTtoPowSpec(){

}
__device__ void dev_powerFramesToLogEnergies(){

}
__device__ void dev_logEnergyToMFCC(){

}
__global__ void kernel_getMfccCoefs(){
	int tId = threadIdx.x + blockIdx.x * blockDim.x;
	if (tId < 1){
		printf("%d %d\n", tId, dev_wave_params.sample_rate);
	}
}

CUDASignalPreprocessor::CUDASignalPreprocessor(float* buffer,int buffer_len, int _frame_len, float overlap_percentage, int sr){
	assert(overlap_percentage >= 0.f && overlap_percentage < 1.f);
	assert(buffer_len >= _frame_len);
	this->wave_params.signal_buffer = buffer;
	cudaMalloc(&(this->wave_params.dev_signal_buffer), sizeof(float)*buffer_len);
	this->wave_params.raw_buffer_len = buffer_len;
	this->wave_params.frame_len = _frame_len;
	this->wave_params.in_frame_offset = (int) (_frame_len*(1.0f - overlap_percentage));

	int i=0;
	while(buffer_len >= _frame_len + i*this->wave_params.in_frame_offset) i++;
	this->wave_params.frame_count = i;

	cudaMalloc(&(this->wave_params.frames), sizeof(float)*this->wave_params.frame_count*_frame_len);
	cudaMalloc(&(this->wave_params.power_frames), sizeof(float)*this->wave_params.frame_count*_frame_len);

	/*
		KISS FFT allocate
		*/
	//this->wave_params.fft_cfg = kiss_fftr_alloc(_frame_len,0,NULL, NULL);
	this->wave_params.sample_rate = sr;
	this->wave_params.base_freq = (float)sr/(float)this->wave_params.frame_len;

}
void CUDASignalPreprocessor::getMfccCoefs(int* nframes,int* pN_mfcc_coefficients,float* output){
	assert(nframes && pN_mfcc_coefficients && output != NULL);
	cudaMemcpy(this->wave_params.dev_signal_buffer, this->wave_params.signal_buffer, sizeof(float)*this->wave_params.raw_buffer_len, cudaMemcpyHostToDevice);
	kernel_getMfccCoefs<<<1,1024>>>();
	cudaDeviceSynchronize();
	/*
	this->applyPreEmphasis(0.97f);
    this->dumpToFrames();
    this->applyWindowsToFrames();
    this->framesFFTtoPowSpec();
    this->powerFramesToLogEnergies();
    this->logEnergyToMFCC();

    *nframes = this->wave_params.frame_count;
    *pN_mfcc_coefficients = this->wave_params.n_mfcc_coefficients;
    memcpy(output, this->wave_params.mfcc_frames, sizeof(float)*this->wave_params.frame_count*this->wave_params.n_mfcc_coefficients);
    */
}

int CUDASignalPreprocessor::getFrameCount(void){
	return this->wave_params.frame_count;
}
int CUDASignalPreprocessor::getMfccCount(void){
	return this->wave_params.n_mfcc_coefficients;
}

void CUDASignalPreprocessor::applyPreEmphasis(float preemphasis){
	for ( 
		int i = this->wave_params.raw_buffer_len-1 ; 
		i > 0; 
		i--
		)
	{
		this->wave_params.signal_buffer[i] = this->wave_params.signal_buffer[i] - preemphasis* this->wave_params.signal_buffer[i-1];
	}
}
void CUDASignalPreprocessor::dumpToFrames(void){
	for (int i = 0; i < this->wave_params.frame_count; ++i)
	{
		memcpy(this->wave_params.frames[i], this->wave_params.signal_buffer + i*this->wave_params.in_frame_offset, this->wave_params.frame_len*sizeof(float)); //4 bytes per float
	}
}
void CUDASignalPreprocessor::applyWindowsToFrames(void){
	float window_buffer[this->wave_params.frame_len];

	for (int i = 0; i < this->wave_params.frame_count; ++i)
	{
		for (int j = 0; j < this->wave_params.frame_len; ++j)
		{
			window_buffer[j]=this->wave_params.frames[i][j] * this->wave_params.hamming_window[j];
		}
		memcpy(this->wave_params.frames[i], window_buffer, this->wave_params.frame_len*sizeof(float)); 
	}
}
void CUDASignalPreprocessor::framesFFTtoPowSpec(void){
//	kiss_fft_cpx complex_arr[this->wave_params.frame_len];
	for (int i = 0; i < this->wave_params.frame_count; ++i)
	{
		//kiss_fftr(this->wave_params.fft_cfg, this->wave_params.frames[i], complex_arr);
		for (int j = 0; j < this->wave_params.frame_len; ++j)
		{
			//this->wave_params.power_frames[i][j] = (pow(complex_arr[j].r,2) + pow(complex_arr[j].i,2))/this->wave_params.frame_len;
		}
	}
}
inline float CUDASignalPreprocessor::melToHz(float mel){
	return 700*(exp(mel/1125)-1);
}
inline float CUDASignalPreprocessor::hzToMel(float f){
	return 1125*log(1+f/700);
}
void CUDASignalPreprocessor::buildFilterBanks(int nfilters,int f0, int fmax){
	this->wave_params.mel_values = new float[nfilters+2];
	this->wave_params.freq_values = new float[nfilters+2];
	this->wave_params.n_mel_filters = nfilters;

	this->wave_params.log_energy_frames = new float*[this->wave_params.frame_count];
	for (int j = 0; j < this->wave_params.frame_count; ++j)
	{
		this->wave_params.log_energy_frames[j] = new float[this->wave_params.n_mel_filters];
	}

	float mel_i = hzToMel(f0);
	float mel_f = hzToMel(fmax);
	for (int i = 0; i < nfilters+2; ++i)
	{
		this->wave_params.mel_values[i] = mel_i + i*(mel_f - mel_i)/(nfilters+1);
		this->wave_params.freq_values[i] = melToHz(this->wave_params.mel_values[i]);
	}
}
float CUDASignalPreprocessor::filterValue(int bank_index, float power_freq){
	assert(this->wave_params.freq_values != NULL && bank_index <= this->wave_params.n_mel_filters && bank_index >0);
	if (power_freq < this->wave_params.freq_values[bank_index-1]) return 0.f;
	else if (
		power_freq > this->wave_params.freq_values[bank_index-1]
		&&
		power_freq <= this->wave_params.freq_values[bank_index]
	)
	{
		return (power_freq -this->wave_params.freq_values[bank_index-1])/(this->wave_params.freq_values[bank_index]-this->wave_params.freq_values[bank_index-1]);
	}
	else if (
		power_freq > this->wave_params.freq_values[bank_index]
		&&
		power_freq <= this->wave_params.freq_values[bank_index+1]
		)
	{
		return ( this->wave_params.freq_values[bank_index+1] - power_freq )/(this->wave_params.freq_values[bank_index+1]-this->wave_params.freq_values[bank_index]);
	}
	else return 0.f;

}

void CUDASignalPreprocessor::powerFramesToLogEnergies(void){
	for (int i = 0; i < this->wave_params.frame_count; ++i)
	{
		for (int filter_index = 0; filter_index < this->wave_params.n_mel_filters; ++filter_index)
		{
			this->wave_params.log_energy_frames[i][filter_index]=0;
			for (int j = 0;
				j < 1 + this->wave_params.frame_len/2 
				&& 
				j*this->wave_params.base_freq< this->wave_params.freq_values[this->wave_params.n_mel_filters+1]; //gone past filter frequency so all ceroes
				++j)
			{
				float actual_freq = j * this->wave_params.base_freq;
				this->wave_params.log_energy_frames[i][filter_index] += this->wave_params.power_frames[i][j] * filterValue(filter_index+1,actual_freq);
			}
			this->wave_params.log_energy_frames[i][filter_index] = 20 * log10( this->wave_params.log_energy_frames[i][filter_index] );
		}
	}
}

void CUDASignalPreprocessor::configureMFCC(int _n_mfcc_coefficients){
	assert(_n_mfcc_coefficients <= this->wave_params.n_mel_filters);
	this->wave_params.n_mfcc_coefficients = _n_mfcc_coefficients;
	this->wave_params.mfcc_frames = new float[this->wave_params.frame_count * _n_mfcc_coefficients];

	cudaMemcpyToSymbol(&dev_wave_params, &(this->wave_params), sizeof(wave_params_t), 0, cudaMemcpyHostToDevice);
}
void CUDASignalPreprocessor::logEnergyToMFCC(void){
	float C_n =0;
	for (int f_indx = 0; f_indx < this->wave_params.frame_count; ++f_indx)
	{
		for (int n = 0; n < this->wave_params.n_mfcc_coefficients; ++n)
		{
			C_n =0;
			for (int k = 0; k < this->wave_params.n_mfcc_coefficients; ++k)
			{
				C_n += this->wave_params.log_energy_frames[f_indx][k] * cos(n*(k-0.5f)*M_PI/this->wave_params.n_mfcc_coefficients);
			}
			this->wave_params.mfcc_frames[this->wave_params.n_mfcc_coefficients*f_indx + n] = C_n;
		}
	}
}

