build:	main.cpp CommandLineParser.cpp ChatClient.cpp
	g++ -o ipk24chat-client main.cpp CommandLineParser.cpp ChatClient.cpp

clean:
	rm ipk24chat-client