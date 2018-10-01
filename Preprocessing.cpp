#include "Preprocessing.h"

SignalPreprocessor::SignalPreprocessor(float* buffer,int buffer_len, int frame_len, float overlap_percentage){
	assert(overlap_percentage >= 0.f && overlap_percentage < 1.f);
	assert(buffer_len >= frame_len);
	this->signal_buffer = buffer;
	this->raw_buffer_len = buffer_len;
	this->frame_len = frame_len;
	this->in_frame_offset = (int) (frame_len*(1.0f - overlap_percentage));

	int i=0;
	while(buffer_len >= frame_len + i*this->in_frame_offset) i++;
	this->frame_count = i;

}

int SignalPreprocessor::getFrameCount(void){
	return this->frame_count;
}