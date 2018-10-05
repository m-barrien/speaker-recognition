DATATYPE=float
TYPEFLAGS=-Dkiss_fft_scalar=$(DATATYPE)
WARNINGS=-W -Wall  -Wcast-align -Wcast-qual  -Wshadow  -Wwrite-strings

FFTUTIL=fft_$(DATATYPE)
FASTFILT=fastconv_$(DATATYPE)
FASTFILTREAL=fastconvr_$(DATATYPE)
PSDPNG=psdpng_$(DATATYPE)
DUMPHDR=dumphdr_$(DATATYPE)


CFLAGS=-std=c++11 -Wall -O3 $(WARNINGS)

all: clean compile

clean:
	rm capture dump_mic dump_mfcc -f
compile:
	g++ $(CFLAGS) -I.. $(TYPEFLAGS) -DREAL_FASTFIR -o capture  main.cpp Preprocessing.cpp include/kissfft/kiss_fft.c include/kissfft/kiss_fftr.c udpsender.cpp -lasound -lpthread
	g++ $(CFLAGS) -I.. $(TYPEFLAGS) -DREAL_FASTFIR -o dump_mic  dump_mic.cpp -lasound -lpthread
	g++ $(CFLAGS) -I.. $(TYPEFLAGS) -DREAL_FASTFIR -o dump_mfcc  dump_mfcc.cpp Preprocessing.cpp include/kissfft/kiss_fft.c include/kissfft/kiss_fftr.c -lasound -lpthread
run:
	./capture
sample:
	rm -f audio.raw out.wav
	./capture > audio.raw
convert_sample:
	ffmpeg -f s16le -ac 1 -ar 44100 -i audio.raw  out.wav
