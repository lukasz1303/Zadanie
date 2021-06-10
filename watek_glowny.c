#include "main.h"
#include "watek_glowny.h"

void mainLoop()
{
    srandom(rank);
    while (1) {
        int perc = random()%100; 

        if (perc<STATE_CHANGE_PROB) {
            if (stan==STAN1_START) {
				ack_f_counter = 0;
				debug("Wysyłam REQ_F z wartoscią priorytetu: %d", &lamport);
				//changeState(STAN1_KONIEC);
				packet_t *pkt = malloc(sizeof(packet_t));
				pkt->data = lamport;
				sleep( SEC_IN_STATE); // to nam zasymuluje, że wiadomość trochę leci w kanale
											  // bez tego algorytm formalnie błędny za każdym razem dawałby poprawny wynik
				for (int i = 0; i < size; i++) {
					if (i != rank){
						sendPacket(pkt, i, REQ_F);
					}
				}
				while (ack_f_counter < size - shop_size);
				changeState(STAN1_SEKCJA);
				debug("Wchodzę do sekcji krytycznej - SKLEP FIRMOWY");
            } else if(stan==STAN1_SEKCJA) {
				sleep(SEC_IN_STATE);
				changeState(STAN1_KONIEC);
				debug("Wychodzę z sekcji krytycznej - SKLEP FIRMOWY");
			}
			else if(stan==STAN1_KONIEC) {
				for (int i = 0; i < ack_f_queue_cur_size; i++) {
					packet_t* pkt = malloc(sizeof(packet_t));
					pkt->data = 1;
					sleep(SEC_IN_STATE);			
					sendPacket(pkt, ack_f_queue[i], ACK_F);
					changeState(STAN1_START);
				}
				ack_f_queue_cur_size = 0;

				debug("Przechodzę do stan1");
			}
			else {

			}
        }
        sleep(SEC_IN_STATE);
    }
}
