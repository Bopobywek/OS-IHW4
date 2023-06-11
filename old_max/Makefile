all: server first_gardener second_gardener observer

server: server.c common.c common.h
	gcc server.c common.c -o server -lrt -lpthread

first_gardener: first_gardener.c common.c common.h
	gcc first_gardener.c common.c -o first_gardener

second_gardener: second_gardener.c common.c common.h
	gcc second_gardener.c common.c -o second_gardener

observer: observer.c common.c common.h
	gcc observer.c common.c  -o observer

clean:
	rm server first_gardener second_gardener observer