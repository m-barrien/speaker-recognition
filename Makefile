all: clean compile

clean:
	rm capture
compile:
	g++ -o capture  main.cpp -lasound
run:
	./capture
sample:
	./capture > audio.raw
	ffmpeg -f s16le -ac 1 -ar 44100 -i audio.raw  out.wav
