#include "main.h"
#include "watek_komunikacyjny.h"

/* wątek komunikacyjny; zajmuje się odbiorem i reakcją na komunikaty */
void* startKomWatek(void* ptr)
{
    MPI_Status status;
    int is_message = FALSE;
    int current_lamport;

    packet_t pakiet;
    /* Obrazuje pętlę odbierającą pakiety o różnych typach */
    while (1) {
        debug("czekam na recv");
        MPI_Recv(&pakiet, 1, MPI_PAKIET_T, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        current_lamport = lamport;
        incBiggerLamport(pakiet.ts);
        debug("Aktualizuje swój lamport %d na %d", current_lamport, lamport);

        switch (status.MPI_TAG) {
        case REQ_F:
            debug("Dostałem REQ_F od %d z danymi %d", pakiet.src, pakiet.data);
            if ((stan != STAN1_REQ && stan != STAN1_SEKCJA && stan != STAN1_KONIEC) ||
                (stan == STAN1_REQ && (priority > pakiet.data ||
                    priority == pakiet.data && pakiet.src < rank))) {
                packet_t* pkt = malloc(sizeof(packet_t));
                pkt->data = 1;
                sleep(SEC_IN_STATE);
                sendPacket(pkt, pakiet.src, ACK_F);
                debug("Wysyłam ACK_F do %d", pakiet.src);
            }
            else {
                ack_f_queue[ack_f_queue_cur_size] = pakiet.src;
                ack_f_queue_cur_size++;
            }

            break;
        case ACK_F:
            debug("Dostałem ACK_F od %d z danymi %d", pakiet.src, pakiet.data);
            ack_f_counter++;
            break;
        case REQ_M:
            debug("Dostałem REQ_M od %d z priorytetem %d", pakiet.src, pakiet.data);
            medium_request_queue[medium_request_queue_cur_size].rank = pakiet.src;
            medium_request_queue[medium_request_queue_cur_size].rel = false;
            medium_request_queue_cur_size++;
            if (rank == pakiet.src) {
                m_pos = medium_request_queue_cur_size - 1;
                if (medium_request_queue_cur_size <= 2){
                    last = rank;
                }
                else {
                    last = medium_request_queue[m_pos-2]
                }
            }

            break;
        case REL_M:
            debug("Dostałem REL_M od %d z danymi %d", pakiet.src, pakiet.data);
            for (int i = 0; i < medium_request_queue_cur_size; i++) {
                if (medium_request_queue[i].rank == pakiet.src) {
                    medium_request_queue[i].rel = true;
                }
            }
            
            break;
          
	    default:
	    break;
        }
    }
}
