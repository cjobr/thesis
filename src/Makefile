.PHONY: all
all: test sim solver rr

test: test.cpp
	g++ -o test test.cpp
sim: simulator.cpp
	g++ -o sim simulator.cpp
solver: localsolver.cpp
	g++ localsolver.cpp -I/opt/localsolver_10_5/include -llocalsolver105 -lpthread -o solver
rr: round-robin.cpp
	g++ -o rr round-robin.cpp

.PHONY: clean
clean:
	-rm test solver sim rr