#include <assert.h>

class SignalPreprocessor
{
public:
	int raw_buffer_len;
	int frame_len;
	int in_frame_offset;
	float *signal_buffer;
	SignalPreprocessor(float*,int,int,float);
	int bar;
};