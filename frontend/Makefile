TARGETS = frontendserver

all: $(TARGETS)

frontendserver: frontendserver.cpp render.cpp frontend_internal.cpp mailserver.cpp createpage.cpp
	g++ -pthread -o frontendserver frontendserver.cpp frontend_internal.cpp render.cpp mailserver.cpp createpage.cpp -std=c++11

clean::
	rm -fv $(TARGETS)
