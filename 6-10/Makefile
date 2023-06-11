all: server first_gardener second_gardener observer

server: server.c common.c
	gcc server.c common.c -o server -lrt -lpthread -g3

first_gardener: first_gardener.c common.c
	gcc first_gardener.c common.c -o first_gardener -g3

second_gardener: second_gardener.c common.c
	gcc second_gardener.c common.c -o second_gardener

observer: observer.c common.c
	gcc observer.c common.c -o observer -lpthread

clean:
	rm server first_gardener second_gardener observer