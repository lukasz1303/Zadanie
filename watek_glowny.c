#include "main.h"
#include "watek_glowny.h"

void mainLoop()
{
	srandom(rank);
	int k;
	while (1) {
		int perc = random() % 100;

		if (perc < STATE_CHANGE_PROB) {
			if (stan == STAN1_START) {
				ack_f_counter = 0;
				//changeState(STAN1_KONIEC);

				sleep(SEC_IN_STATE); // to nam zasymuluje, że wiadomość trochę leci w kanale
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
				while (ack_f_counter < size - shop_size);
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

				debug("Przechodzę do stan2_START");
			}
			else if (stan == STAN2_START) {
				for (int i = 0; i < size; i++) {
					if (msg_received[i] == 1)
						msg_received[i] = 0;
				}
				sleep(SEC_IN_STATE); // to nam zasymuluje, że wiadomość trochę leci w kanale
				packet_t* pkt = malloc(sizeof(packet_t));
				incLamport();
				priority = lamport;
				pkt->data = priority;
				debug("\t\t\tWysyłam REQ_M z wartoscią priorytetu: %d", priority);
				changeState(STAN2_REQ);
				for (int i = 0; i < size; i++) {
					sendPacket2(pkt, i, REQ_M);
				}

				debug("Czekam na odbiór starszej wiadomości");

				while (1) {
					int c = 0;
					int c2 = 0;
					for (int i = 0; i < size; i++) {
						if (msg_received[i] > 0) {
							c++;
						}
						if (msg_received[i] == 2) {
							c2++;
						}
					}
					if (c == size && c2<number_of_Mediums) {
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
					debug("Czekam na odbiór REL_M od poprzedniego użytkownika medium: %d", last);
					while (last_rel == 0);
				}
				//debug("m_pos = %d, last = %d", m_pos, last);
				while (m_pos == -1);
				k = m_pos % 2;
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
				if (mediums[k].c == 0) {
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
				changeState(STAN3_START);
				debug("Przechodzę do stan3_START");
			}
			else if (stan == STAN3_START) {

				if (last != rank) {
					debug("\t\t\tCzekam na odbiór ACK_T od poprzedniego użytkownika tunelu: %d", last);
					while (last_rel_tun == 0);
				}

				changeState(STAN3_SEKCJA);
				debug("\t\t\t\t\tWchodzę do sekcji krytycznej - WYJSCIE Z TUNELU %d", k);
				m_pos = -1;
			}
			else if (stan == STAN3_SEKCJA) {
				sleep(SEC_IN_STATE);
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
				changeState(STAN2_START);
				debug("\t\t\t\t\t\t\tPrzechodzę do stan1_START");


				//|^
				//TODO ZMIENIĆ STAN2_START NA STAN1_START



			}
			else {

			}
		}
		sleep(SEC_IN_STATE);
	}
}
