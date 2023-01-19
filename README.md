# c-apiclient
Veraison API client implementation in C

This example makes use of curl library to make REST API invocations to the Veraison server.
It implements only Synchronous verification part of the challenge response sequence.

Building the Client:  gcc -g -o client client.c  -lcurl

Running the Client example : client -l 127.0.0.1 -t psa-evidence.cbor
