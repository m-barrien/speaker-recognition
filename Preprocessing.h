#include <assert.h>

class SignalPreprocessor
{
private:
	int raw_buffer_len;
	int frame_len;
	int frame_count;
	int in_frame_offset;
	float *signal_buffer;
public:
	SignalPreprocessor(float*,int,int,float);
	int getFrameCount(void);
};