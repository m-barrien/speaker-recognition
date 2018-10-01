#include "Preprocessing.h"

SignalPreprocessor::SignalPreprocessor(float* buffer,int buffer_len, int frame_len, float overlap_percentage){
	assert(overlap_percentage >= 0.f && overlap_percentage < 1.f);
	assert(buffer_len >= frame_len);
	this->signal_buffer = buffer;
	this->out_signal_buffer = new int16_t[buffer_len];
	this->raw_buffer_len = buffer_len;
	this->frame_len = frame_len;
	this->in_frame_offset = (int) (frame_len*(1.0f - overlap_percentage));

	int i=0;
	while(buffer_len >= frame_len + i*this->in_frame_offset) i++;
	this->frame_count = i;

	this->frames = new float*[this->frame_count];
	for (int i = 0; i < this->frame_count; ++i)
	{
		this->frames[i] = new float[frame_len];
	}

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