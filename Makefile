all: clean compile

clean:
	rm capture -f
compile:
	g++ -std=c++11 -o capture  main.cpp Preprocessing.cpp -lasound -lpthread
run:
	./capture
sample:
	rm -f audio.raw out.wav
	./capture > audio.raw
convert_sample:
	ffmpeg -f s16le -ac 1 -ar 44100 -i audio.raw  out.wav
