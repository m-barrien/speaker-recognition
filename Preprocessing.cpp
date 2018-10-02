#include "Preprocessing.h"

SignalPreprocessor::SignalPreprocessor(float* buffer,int buffer_len, int _frame_len, float overlap_percentage, int sr){
	assert(overlap_percentage >= 0.f && overlap_percentage < 1.f);
	assert(buffer_len >= _frame_len);
	this->signal_buffer = buffer;
	this->out_signal_buffer = new int16_t[buffer_len];
	this->raw_buffer_len = buffer_len;
	this->frame_len = _frame_len;
	this->in_frame_offset = (int) (_frame_len*(1.0f - overlap_percentage));

	int i=0;
	while(buffer_len >= _frame_len + i*this->in_frame_offset) i++;
	this->frame_count = i;

	this->frames = new float*[this->frame_count];
	this->power_frames = new float*[this->frame_count];
	for (int j = 0; j < this->frame_count; ++j)
	{
		this->frames[j] = new float[_frame_len];
		this->power_frames[j] = new float[_frame_len];
	}
	/*
		Fill hamming window
	*/
	this->hamming_window = new float[_frame_len];
	for (int j = 0; j < _frame_len; ++j)
	{
		this->hamming_window[j]=( 0.54 - 0.46 * cos((2*M_PI*j)/(this->frame_len-1)) );
	}

	/*
		KISS FFT allocate
		*/
	this->fft_cfg = kiss_fftr_alloc(_frame_len,0,NULL, NULL);
	this->sample_rate = sr;
	this->base_freq = (float)sr/(float)frame_len;

}

int SignalPreprocessor::getFrameCount(void){
	return this->frame_count;
}

void SignalPreprocessor::applyPreEmphasis(float preemphasis){
	for ( 
		int i = this->raw_buffer_len-1 ; 
		i > 0; 
		i--
		)
	{
		this->signal_buffer[i] = this->signal_buffer[i] - preemphasis* this->signal_buffer[i-1];
	}
}
void SignalPreprocessor::dumpSignal(char* out_addr){
	for (int i = 0; i < raw_buffer_len; ++i)
	{
		this->out_signal_buffer[i] = (int16_t) (this->signal_buffer[i]*32767.f);
	}
	memcpy(out_addr, this->out_signal_buffer, this->raw_buffer_len*2);
}
void SignalPreprocessor::dumpToFrames(void){
	for (int i = 0; i < this->frame_count; ++i)
	{
		memcpy(this->frames[i], this->signal_buffer + i*in_frame_offset, this->frame_len*sizeof(float)); //4 bytes per float
	}
}
void SignalPreprocessor::applyWindowsToFrames(void){
	float window_buffer[this->frame_len];

	for (int i = 0; i < this->frame_count; ++i)
	{
		for (int j = 0; j < this->frame_len; ++j)
		{
			window_buffer[j]=this->frames[i][j] * this->hamming_window[j];
		}
		memcpy(this->frames[i], window_buffer, this->frame_len*sizeof(float)); 
	}
}
void SignalPreprocessor::framesFFTtoPowSpec(void){
	kiss_fft_cpx complex_arr[this->frame_len];
	for (int i = 0; i < this->frame_count; ++i)
	{
		kiss_fftr(this->fft_cfg, this->frames[i], complex_arr);
		for (int j = 0; j < this->frame_len; ++j)
		{
			this->power_frames[i][j] = (pow(complex_arr[j].r,2) + pow(complex_arr[j].i,2))/this->frame_len;
		}
	}
}
float* SignalPreprocessor::getFrame(int i){
	return this->frames[i];
}
float* SignalPreprocessor::getPowerFrame(int i){
	return this->power_frames[i];
}
inline float SignalPreprocessor::melToHz(float mel){
	return 700*(exp(mel/1125)-1);
}
inline float SignalPreprocessor::hzToMel(float f){
	return 1125*log(1+f/700);
}
void SignalPreprocessor::buildFilterBanks(int nfilters,int f0, int fmax){
	this->mel_values = new float[nfilters+2];
	this->freq_values = new float[nfilters+2];
	this->n_mel_filters = nfilters;

	this->log_energy_frames = new float*[this->frame_count];
	for (int j = 0; j < this->frame_count; ++j)
	{
		this->log_energy_frames[j] = new float[this->n_mel_filters];
	}

	float mel_i = hzToMel(f0);
	float mel_f = hzToMel(fmax);
	for (int i = 0; i < nfilters+2; ++i)
	{
		this->mel_values[i] = mel_i + i*(mel_f - mel_i)/(nfilters+1);
		this->freq_values[i] = melToHz(this->mel_values[i]);
	}
}
float SignalPreprocessor::filterValue(int bank_index, float power_freq){
	assert(this->freq_values != NULL && bank_index <= this->n_mel_filters && bank_index >0);
	if (power_freq < this->freq_values[bank_index-1]) return 0.f;
	else if (
		power_freq > this->freq_values[bank_index-1]
		&&
		power_freq <= this->freq_values[bank_index]
	)
	{
		return (power_freq -this->freq_values[bank_index-1])/(this->freq_values[bank_index]-this->freq_values[bank_index-1]);
	}
	else if (
		power_freq > this->freq_values[bank_index]
		&&
		power_freq <= this->freq_values[bank_index+1]
		)
	{
		return ( this->freq_values[bank_index+1] - power_freq )/(this->freq_values[bank_index+1]-this->freq_values[bank_index]);
	}
	else return 0.f;

}

void SignalPreprocessor::powerFramesToEnergies(void){
	for (int i = 0; i < this->frame_count; ++i)
	{
		for (int filter_index = 0; filter_index < this->n_mel_filters; ++filter_index)
		{
			this->log_energy_frames[i][filter_index]=0;
			for (int j = 0;
				j < 1 + this->frame_len/2 
				&& 
				j*this->base_freq< this->freq_values[this->n_mel_filters+1]; //gone past filter frequency so all ceroes
				++j)
			{
				float actual_freq = j * this->base_freq;
				this->log_energy_frames[i][filter_index] += this->power_frames[i][j] * filterValue(filter_index+1,actual_freq);
			}
			this->log_energy_frames[i][filter_index] = 20 * log10( this->log_energy_frames[i][filter_index] );
			std::cout << this->log_energy_frames[i][filter_index] << " ";
		}
		std::cout << std::endl;
	}
}