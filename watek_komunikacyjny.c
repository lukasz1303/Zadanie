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
        MPI_Recv(&pakiet, 1, MPI_PAKIET_T, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        current_lamport = lamport;
        incBiggerLamport(pakiet.ts);
        
        //zaznacz odebranie starszej wiadomoci, innej od REQ_M)
        if (status.MPI_TAG != REQ_M && pakiet.ts >= current_lamport) {
            msg_received[pakiet.src] = 1;
        }

        switch (status.MPI_TAG) {
        case REQ_F:
            debug("Dostałem REQ_F od %d z danymi %d", pakiet.src, pakiet.data);
            // sprawdź priorytet otrzymanego żądania
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
            // zaznacz otrzymanie wiadomości REQ_M
            if (pakiet.data < priority || pakiet.data == priority && pakiet.src < rank || stan != STAN2_REQ)
                msg_received[pakiet.src] = 2;
            else
                msg_received[pakiet.src] = 3;

            debug("Dostałem REQ_M od %d z priorytetem %d", pakiet.src, pakiet.data);
            medium_request_queue[medium_request_queue_cur_size].rank = pakiet.src;
            medium_request_queue[medium_request_queue_cur_size].rel = 0;
            medium_request_queue[medium_request_queue_cur_size].priority = pakiet.data;
            medium_request_queue[medium_request_queue_cur_size].rel_tun = 0;

            medium_request_queue_cur_size++;
            //posortuj kolejkę żądań po priorytetach
            qsort(medium_request_queue, medium_request_queue_cur_size, sizeof(process), comparePriority);

            //ustal moją pozycję w kolejce żądań
            for (int i = medium_request_queue_cur_size - 1; i >= 0; i--) {
                if (medium_request_queue[i].rank == rank && medium_request_queue[i].rel == 0) {
                    m_pos = i;
                    break;
                }
            }

            if (m_pos != -1) {
                if (m_pos < number_of_Mediums) {
                    last = rank;
                    last_rel = 1;
                    last_rel_tun = 1;
                }
                else {
                    last = medium_request_queue[m_pos - number_of_Mediums].rank;
                    last_rel = medium_request_queue[m_pos - number_of_Mediums].rel;
                    last_rel_tun = medium_request_queue[m_pos - number_of_Mediums].rel_tun;
                }
            }

            break;
        case REL_M:
            debug("Dostałem REL_M od %d z danymi %d", pakiet.src, pakiet.data);
            for (int i = 0; i < medium_request_queue_cur_size; i++) {
                if (medium_request_queue[i].rank == pakiet.src) {
                    medium_request_queue[i].rel = 1;
                }
            }
            if (pakiet.src != rank) {
                mediums[pakiet.data].c--;
            }
            if (m_pos > -1) {
                if (medium_request_queue[m_pos - number_of_Mediums].rel == 1) {
                    last_rel = 1;
                }
            }
            if (rank == 0) {
                for (int i = 0; i < medium_request_queue_cur_size; ++i) {
                    debug("[%d %d %d %d]", medium_request_queue[i].rank, medium_request_queue[i].rel, medium_request_queue[i].priority, medium_request_queue[i].rel_tun);
                }
            }
            

            break;
        case ACK_T:
            debug("Dostałem ACK_T od %d z danymi %d", pakiet.src, pakiet.data);
            for (int i = 0; i < medium_request_queue_cur_size; i++) {
                if (medium_request_queue[i].rank == pakiet.src) {
                    medium_request_queue[i].rel_tun = 1;
                }
            }
            if (m_pos > -1) {
                if (medium_request_queue[m_pos - number_of_Mediums].rel_tun == 1) {
                    last_rel_tun = 1;

                }
            }

            int rel_counter = 0;
            for (int i = 0; i < 2 * number_of_Mediums; i++) {
                if (medium_request_queue[i].rel == 1 && medium_request_queue[i].rel_tun == 1) {
                    rel_counter++;
                }
                else {
                    break;
                }
            }

            if (rel_counter >= 2 * number_of_Mediums) {
                for (int i = number_of_Mediums; i < medium_request_queue_cur_size; i++) {
                    medium_request_queue[i - number_of_Mediums] = medium_request_queue[i];
                }
                medium_request_queue_cur_size -= number_of_Mediums;
                if (m_pos >= number_of_Mediums) {
                    m_pos -= number_of_Mediums;
                }
            }
           
            if (rank == 0) {
                for (int i = 0; i < medium_request_queue_cur_size; ++i) {
                    debug("[%d %d %d %d]", medium_request_queue[i].rank, medium_request_queue[i].rel, medium_request_queue[i].priority, medium_request_queue[i].rel_tun);
                }
            }


            break;
        default:
            break;
        }
    }
}
