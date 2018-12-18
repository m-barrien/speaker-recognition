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
compile:
	g++ $(CFLAGS) -I.. $(TYPEFLAGS) -DREAL_FASTFIR -o bin/dump_mfcc  dump_mfcc.cpp Preprocessing.cpp include/kissfft/kiss_fft.c include/kissfft/kiss_fftr.c -lpthread
	/usr/local/cuda-10.0/bin/nvcc -ccbin g++ -I../../common/inc  -m64    -gencode arch=compute_61,code=sm_61 -gencode arch=compute_70,code=sm_70 -gencode arch=compute_75,code=sm_75 -gencode arch=compute_75,code=compute_75 -o main.o -c main.cu -lcufft
	/usr/local/cuda-10.0/bin/nvcc -ccbin g++   -m64      -gencode arch=compute_61,code=sm_61 -gencode arch=compute_70,code=sm_70 -gencode arch=compute_75,code=sm_75 -gencode arch=compute_75,code=compute_75 -o main main.o 
run:
	./bin/dump_mfcc
sample:
	rm -f audio.raw out.wav
	./bin/capture > ../audio.raw
convert_sample:
	ffmpeg -f s16le -ac 1 -ar 44100 -i audio.raw  out.wav
gen_cert:
	openssl req -x509 -newkey rsa:2048 -keyout doorserver/crt/key.pem -out doorserver/crt/cert.pem -days 365
