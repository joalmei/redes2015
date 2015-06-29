all:
	g++ -o main main.cc messenger.cc -pthread -g

run:
	./main

zip:
	zip -r redes.zip Makefile contact.hh group.hh messenger.hh messenger.cc main.cc

clean:
	rm main