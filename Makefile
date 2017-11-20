all: latency_pxmsg bandwidth_pxmsg latency_udpsocket_client latency_udpsocket_server latency_unixsocket_client latency_unixsocket_server

latency_pxmsg: latency_pxmsg.c
	gcc -g -O0 -D_REENTRANT -Wall $^ -o $@ -lrt

bandwidth_pxmsg: bandwidth_pxmsg.c
	gcc -g -O0 -D_REENTRANT -Wall $^ -o $@ -lrt

latency_udpsocket_client: latency_udpsocket_client.c
	gcc -g -O0 -D_REENTRANT -Wall $^ -o $@

latency_udpsocket_server: latency_udpsocket_server.c
	gcc -g -O0 -D_REENTRANT -Wall $^ -o $@

latency_unixsocket_client: latency_unixsocket_client.c
	gcc -g -O0 -D_REENTRANT -Wall $^ -o $@

latency_unixsocket_server: latency_unixsocket_server.c
	gcc -g -O0 -D_REENTRANT -Wall $^ -o $@

clean:
	rm -rf *~ latency_pxmsg bandwidth_pxmsg latency_udpsocket_client latency_udpsocket_server latency_unixsocket_client latency_unixsocket_server
