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
	rm bin/capture bin/dump_mic bin/dump_mfcc -f

compile: ensure_bin
	g++ $(CFLAGS) -I.. $(TYPEFLAGS) -DREAL_FASTFIR -o ./bin/capture  main.cpp Preprocessing.cpp include/kissfft/kiss_fft.c include/kissfft/kiss_fftr.c udpsender.cpp -lasound -lpthread
	g++ $(CFLAGS) -I.. $(TYPEFLAGS) -DREAL_FASTFIR -o ./bin/wav2mfcc-json  convert_wav_to_mfcc_json.cpp Preprocessing.cpp include/kissfft/kiss_fft.c include/kissfft/kiss_fftr.c udpsender.cpp -lasound -lpthread -lsndfile
	g++ $(CFLAGS) -I.. $(TYPEFLAGS) -DREAL_FASTFIR -o ./bin/dump_mic  dump_mic.cpp -lasound -lpthread
	g++ $(CFLAGS) -I.. $(TYPEFLAGS) -DREAL_FASTFIR -o ./bin/dump_mfcc  dump_mfcc.cpp Preprocessing.cpp include/kissfft/kiss_fft.c include/kissfft/kiss_fftr.c -lasound -lpthread

ensure_bin:
	mkdir -p bin

run:
	./capture
sample:
	rm -f audio.raw out.wav
	./bin/capture > ../audio.raw
convert_sample:
	ffmpeg -f s16le -ac 1 -ar 44100 -i audio.raw  out.wav
gen_cert:
	openssl req -x509 -newkey rsa:2048 -keyout doorserver/crt/key.pem -out doorserver/crt/cert.pem -days 365
