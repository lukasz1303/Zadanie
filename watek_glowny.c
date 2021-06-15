#include "main.h"
#include "watek_glowny.h"
#include "watek_do_odp_medium.h"

void mainLoop()
{
	srandom(rank);
	int k;
	while (1) {
		int perc = random() % 100;

		if (perc < STATE_CHANGE_PROB) {
			if (stan == STAN1_START) {
				ack_f_counter = 0;

				sleep(SEC_IN_STATE);
				packet_t* pkt = malloc(sizeof(packet_t));
				incLamport();
				priority = lamport;
				pkt->data = priority;
				debug("Wysyłam REQ_F z wartoscią priorytetu: %d", priority);
				changeState(STAN1_REQ);
				for (int i = 0; i < size; i++) {
					if (i != rank) {
						sendPacket2(pkt, i, REQ_F);
					}
				}
				while (ack_f_counter < size - shop_size); // sprawdź czy jest miejsce w sklepie
				changeState(STAN1_SEKCJA);
				debug("Wchodzę do sekcji krytycznej - SKLEP FIRMOWY");
			}
			else if (stan == STAN1_SEKCJA) {
				sleep(SEC_IN_STATE);
				changeState(STAN1_KONIEC);
				debug("Wychodzę z sekcji krytycznej - SKLEP FIRMOWY");
			}
			else if (stan == STAN1_KONIEC) {
				incLamport();
				for (int i = 0; i < ack_f_queue_cur_size; i++) {
					packet_t* pkt = malloc(sizeof(packet_t));
					pkt->data = 1;
					sleep(SEC_IN_STATE);
					sendPacket2(pkt, ack_f_queue[i], ACK_F);
					debug("Wysyłam ACK_F z kolejki do %d", ack_f_queue[i]);

				}
				changeState(STAN2_START);
				ack_f_queue_cur_size = 0;

				debug("Przechodzę do STAN 2");
			}
			else if (stan == STAN2_START) {
				for (int i = 0; i < size; i++) {
					if (msg_received[i] == 1)
						msg_received[i] = 0;
				}
				sleep(SEC_IN_STATE);
				packet_t* pkt = malloc(sizeof(packet_t));
				incLamport();
				priority = lamport;
				pkt->data = priority;
				debug("\t\t\tWysyłam REQ_M z wartoscią priorytetu: %d", priority);
				changeState(STAN2_REQ);
				for (int i = 0; i < size; i++) {
					sendPacket2(pkt, i, REQ_M);
				}

				// czekanie na odbiór starszej wiadomości od wszystkich procesów
				while (1) {
					int c = 0;
					for (int i = 0; i < size; i++) {
						if (msg_received[i] > 0) {
							c++;
						}
					}
					if (c == size) {
						break;
					}
				}

				for (int i = 0; i < size; i++) {
					if (msg_received[i] == 3)
						msg_received[i] = 2;
					else
						msg_received[i] = 0;
				}

				if (last != rank) {
					debug("Czekam na odbiór REL_M od poprzedniego użytkownika medium");
					while (last_rel == 0);
				}

				while (m_pos == -1);
				//wybór medium
				k = m_pos % number_of_Mediums;
				mediums[k].c--;
				changeState(STAN2_SEKCJA);
				debug("Wchodzę do sekcji krytycznej - OTWARCIE TUNELU PRZEZ MEDIUM %d", k);
			}
			else if (stan == STAN2_SEKCJA) {
				sleep(SEC_IN_STATE);
				changeState(STAN2_KONIEC);
				debug("Wychodzę z sekcji krytycznej - OTWARCIE TUNELU PRZEZ MEDIUM %d", k);
			}
			else if (stan == STAN2_KONIEC) {
				pthread_t threadRest;
				int *k_send = malloc(sizeof(*k_send));
				*k_send = k;
				pthread_create(&threadRest, NULL, startRestWatek, (void*)k_send);
				changeState(STAN3_START);
				debug("Przechodzę do STAN 3");
			}
			else if (stan == STAN3_START) {

				sleep(rand()%6+1);
				if (last != rank) {
					if (last_rel_tun == 0) {
						debug("\t\t\tCzekam na odbiór ACK_T od poprzedniego użytkownika tunelu");
					}
					while (last_rel_tun == 0);
				}

				changeState(STAN3_SEKCJA);
				debug("\t\t\t\t\tWchodzę do sekcji krytycznej - WYJSCIE Z TUNELU %d", k);
				m_pos = -1;
				last = -1;
			}
			else if (stan == STAN3_SEKCJA) {
				changeState(STAN3_KONIEC);
				debug("\t\t\t\t\tWychodzę z sekcji krytycznej - WYJSCIE Z TUNELU %d", k);
			}
			else if (stan == STAN3_KONIEC) {

				incLamport();
				for (int i = 0; i < size; i++) {
					packet_t* pkt = malloc(sizeof(packet_t));
					pkt->data = 1;
					sleep(SEC_IN_STATE);
					sendPacket2(pkt, i, ACK_T);
					debug("\t\t\t\t\t\t\tWysyłam ACK_T do %d", i);

				}
				changeState(STAN1_START);
				debug("\t\t\t\t\t\t\tPrzechodzę do STAN 1");

			}
			else {}
		}
		sleep(SEC_IN_STATE);
	}
}
