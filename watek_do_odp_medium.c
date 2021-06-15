#include "main.h"
#include "watek_do_odp_medium.h"

/* wątek do odpoczynku medium; zajmuje się odpoczynkiem medium i wysyłaniem pakietów REL_M do procesów*/
void* startRestWatek(void* ptr)
{
	int k = *((int*)ptr);
	debug("k = %d, mediums[k].c = %d", k, mediums[k].c);
	if (mediums[k].c == 0) {
		debug("Medium %d odpoczywa", k);
		sleep(SEC_IN_STATE);
		mediums[k].c = mediums[k].tun;
		debug("Medium %d skończyło odpoczywać", k);
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
