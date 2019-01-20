LDFLAGS=-static -s -O2 -Wno-unused-result -std=c++11 
.PHONY:	clean
search:	main.cc anqi.cc ClientSocket.cpp Protocol.cpp
	$(CXX) -o $@ $^ $(LDFLAGS)
clean:
	DEL search.exe 2>NUL
