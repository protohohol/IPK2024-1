build:	main.cpp CommandLineParser.cpp
	g++ -o ipk24chat-client main.cpp CommandLineParser.cpp

clean:
	rm ipk24chat-client