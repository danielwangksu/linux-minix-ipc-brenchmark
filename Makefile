all: latency_pxmsg bandwidth_pxmsg latency_udpsocket_client latency_udpsocket_server latency_unixsocket_client latency_unixsocket_server bandwidth_unixsocket bandwidth_udpsocket_client bandwidth_udpsocket_server latency_udp_enc_client latency_udp_enc_server
latency_pxmsg: latency_pxmsg.c
	gcc -g -O0 -D_REENTRANT -Wall $^ -o $@ -lrt

bandwidth_pxmsg: bandwidth_pxmsg.c
	gcc -g -O0 -D_REENTRANT -Wall $^ -o $@ -lrt

bandwidth_unixsocket: bandwidth_unixsocket.c
	gcc -g -O0 -D_REENTRANT -Wall $^ -o $@ -lrt

bandwidth_udpsocket_client: bandwidth_udpsocket_client.c
	gcc -g -O0 -D_REENTRANT -Wall $^ -o $@ -lrt

bandwidth_udpsocket_server: bandwidth_udpsocket_server.c
	gcc -g -O0 -D_REENTRANT -Wall $^ -o $@ -lrt

latency_udpsocket_client: latency_udpsocket_client.c
	gcc -g -O0 -D_REENTRANT -Wall $^ -o $@ -lrt

latency_udpsocket_server: latency_udpsocket_server.c
	gcc -g -O0 -D_REENTRANT -Wall $^ -o $@ -lrt

latency_unixsocket_client: latency_unixsocket_client.c
	gcc -g -O0 -D_REENTRANT -Wall $^ -o $@ -lrt

latency_unixsocket_server: latency_unixsocket_server.c
	gcc -g -O0 -D_REENTRANT -Wall $^ -o $@ -lrt

latency_udp_enc_client: latency_udp_enc_client.c
	gcc -g -O0 -D_REENTRANT -Wall $^ -o $@ -lrt -lssl -lcrypto

latency_udp_enc_server: latency_udp_enc_server.c
	gcc -g -O0 -D_REENTRANT -Wall $^ -o $@ -lrt -lssl -lcrypto

clean:
	rm -rf *~ latency_pxmsg bandwidth_pxmsg latency_udpsocket_client latency_udpsocket_server latency_unixsocket_client latency_unixsocket_server bandwidth_unixsocket bandwidth_udpsocket_client bandwidth_udpsocket_server latency_udp_enc_client latency_udp_enc_server