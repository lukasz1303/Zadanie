#include "main.h"
#include "watek_do_odp_medium.h"

/* wątek komunikacyjny; zajmuje się odbiorem i reakcją na komunikaty */
void* startRestWatek(void* ptr)
{
	int k = *((int*)ptr);
	if (mediums[k].c == 0) {
		sleep(SEC_IN_STATE);
		mediums[k].c = mediums[k].tun;
	}

	incLamport();
	for (int i = 0; i < size; i++) {
		packet_t* pkt = malloc(sizeof(packet_t));
		pkt->data = 1;
		sleep(SEC_IN_STATE);
		sendPacket2(pkt, i, REL_M);
		debug("Wysyłam REL_M do %d", i);

	}
}
