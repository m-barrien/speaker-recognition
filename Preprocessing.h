#include <assert.h>
#include <iostream>

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
	
	/*
		void SignalPreprocessor::applyPreEmphasis(void)

		S1 (n) = S(n) - alpha* S(n-1)
		Where  s  (n)  is  the  speech  signal  &  parameter  α  is  
		usually between  0.94  and  0.97. Pre-emphasis  is  needed  for 
		 high frequency in order to improve phone recognition performance.  
	*/
	void applyPreEmphasis(float);
};