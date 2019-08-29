CC = g++
all: ./src/*.cpp
	$(CC) -g -Wall -I ./include -lwiringPi -o can.out ./src/*.cpp
