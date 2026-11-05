CC=gcc -O3 -fwrapv -Wall -Wextra -g -march=native -mtune=native
CPP=g++ -O3 -fwrapv -Wall -Wextra -g -march=native -mtune=native

all: holtrace-verify holtrace-worker holtrace2ml pt2holtrace

pt2holtrace: pt2holtrace.cc
	$(CPP) -o pt2holtrace pt2holtrace.cc

holtrace2ml: holtrace2ml.c in.o out.o holtrace_base32.o
	$(CC) -o holtrace2ml holtrace2ml.c in.o out.o holtrace_base32.o

holtrace-worker: holtrace-worker.c in.o out.o holtrace_base32.o
	$(CC) -o holtrace-worker holtrace-worker.c in.o out.o holtrace_base32.o

holtrace-verify: holtrace-verify.c in.o
	$(CC) -o holtrace-verify holtrace-verify.c in.o

holtrace_base32.o: holtrace_base32.c
	$(CC) -c holtrace_base32.c

in.o: in.c
	$(CC) -c in.c

out.o: out.c
	$(CC) -c out.c
