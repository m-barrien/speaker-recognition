#include "Preprocessing.h"

SignalPreprocessor::SignalPreprocessor(float* buffer,int buffer_len, int frame_len, float overlap_percentage){
	assert(overlap_percentage >= 0 && overlap_percentage < 1);
	this->signal_buffer = buffer;
	this->raw_buffer_len = buffer_len;
	this->frame_len = frame_len;
}