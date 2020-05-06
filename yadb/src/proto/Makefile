SRCS = $(wildcard *.proto)
OBJS = $(SRCS:.proto=.pb.cc)

all: $(OBJS)
	
clean:
	rm -f *.pb.cc *.pb.h
	
%.pb.cc: %.proto
	protoc --cpp_out=. $<