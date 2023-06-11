all: MulticastReceiver MulticastSender
MulticastReceiver: MulticastReceiver.c DieWithError.c
	gcc MulticastReceiver.c DieWithError.c -o MulticastReceiver
MulticastSender: MulticastSender.c DieWithError.c
	gcc MulticastSender.c DieWithError.c -o MulticastSender
