TARGETS = storageNode master

all: $(TARGETS)

storageNode: storageNode.cc
	g++ storageNode.cc -o store -lpthread -std=c++11

master: master.cc
	g++ master.cc -o master -lpthread -std=c++11

clean::
	rm -fv $(TARGETS)
