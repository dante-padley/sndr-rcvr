main: sender.cpp recv.cpp
	g++ -o sender sender.cpp
	g++ -o receiver recv.cpp
run: sender receiver
	./receiver &
	./sender textfile.txt
clean:
	rm -f sender receiver recvfile