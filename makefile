main: sender.cpp recv.cpp
	g++ -o sender sender.cpp
	g++ -o receiver recv.cpp
clean:
	rm -f sender receiver