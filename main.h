#ifndef GLOBALH
#define GLOBALH

#define _GNU_SOURCE
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
/* odkomentować, jeżeli się chce DEBUGI */
//#define DEBUG 
/* boolean */
#define TRUE 1
#define FALSE 0

/* używane w wątku głównym, determinuje jak często i na jak długo zmieniają się stany */
#define STATE_CHANGE_PROB 50
#define SEC_IN_STATE 2

#define ROOT 0

/* stany procesu */
typedef enum {STAN1_START, STAN1_SEKCJA, STAN1_KONIEC, STAN1_REQ, STAN2_START, STAN2_SEKCJA, STAN2_KONIEC, STAN2_REQ, STAN2_WAIT} state_t;
extern state_t stan;
extern int rank;
extern int size;
extern int priority;

/* Ile mamy łoju na składzie? */
extern int tallow;

/* stan globalny wykryty przez monitor */
extern int globalState;
/* ilu już odpowiedziało na GIVEMESTATE */
extern int numberReceived;
extern int number_of_Mediums;

/* to może przeniesiemy do global... */
typedef struct {
    int ts;       /* timestamp (zegar lamporta */
    int src;      /* pole nie przesyłane, ale ustawiane w main_loop */

    int data;     /* przykładowe pole z danymi; można zmienić nazwę na bardziej pasującą */
} packet_t;
extern MPI_Datatype MPI_PAKIET_T;

/* Typy wiadomości */
#define FINISH 1
#define REQ_F 2
#define ACK_F 3
#define REQ_M 4
#define REL_M 5
#define STATE 6

/* macro debug - działa jak printf, kiedy zdefiniowano
   DEBUG, kiedy DEBUG niezdefiniowane działa jak instrukcja pusta 
   
   używa się dokładnie jak printfa, tyle, że dodaje kolorków i automatycznie
   wyświetla rank

   w związku z tym, zmienna "rank" musi istnieć.

   w printfie: definicja znaku specjalnego "%c[%d;%dm [%d]" escape[styl bold/normal;kolor [RANK]
                                           FORMAT:argumenty doklejone z wywołania debug poprzez __VA_ARGS__
					   "%c[%d;%dm"       wyczyszczenie atrybutów    27,0,37
                                            UWAGA:
                                                27 == kod ascii escape. 
                                                Pierwsze %c[%d;%dm ( np 27[1;10m ) definiuje styl i kolor literek
                                                Drugie   %c[%d;%dm czyli 27[0;37m przywraca domyślne kolory i brak pogrubienia (bolda)
                                                ...  w definicji makra oznacza, że ma zmienną liczbę parametrów
                                            
*/
int incLamport();
int incBiggerLamport(int);
extern int lamport;
extern int ack_f_counter;
extern int ack_f_queue[100];
extern int ack_f_queue_cur_size;
extern int shop_size;
extern int pos;

typedef struct {
    int tun;       /* liczba tuneli jakie medium może otworzyć do odpoczynku */
    int c;      /* pozostała liczba tuneli do otwarcia przed odpoczynkiem */
} medium;

typedef struct process {
    int rank;       /* liczba tuneli jakie medium może otworzyć do odpoczynku */
    int rel;
    int priority;/* pozostała liczba tuneli do otwarcia przed odpoczynkiem */
} process;

extern medium *mediums;

extern process medium_request_queue[100];
extern int medium_request_queue_cur_size;
extern int last;
extern int last_rel;
extern int m_pos;


#ifdef DEBUG
#define debug(FORMAT,...) printf("%c[%d;%dm [%d]:[ts %d] [p %d] " FORMAT "%c[%d;%dm\n",  27, (1+(rank/7))%2, 31+(6+rank)%7, rank, lamport, priority, ##__VA_ARGS__, 27,0,37);
#else
#define debug(...) ;
#endif

#define P_WHITE printf("%c[%d;%dm",27,1,37);
#define P_BLACK printf("%c[%d;%dm",27,1,30);
#define P_RED printf("%c[%d;%dm",27,1,31);
#define P_GREEN printf("%c[%d;%dm",27,1,33);
#define P_BLUE printf("%c[%d;%dm",27,1,34);
#define P_MAGENTA printf("%c[%d;%dm",27,1,35);
#define P_CYAN printf("%c[%d;%d;%dm",27,1,36);
#define P_SET(X) printf("%c[%d;%dm",27,1,31+(6+X)%7);
#define P_CLR printf("%c[%d;%dm",27,0,37);

/* printf ale z kolorkami i automatycznym wyświetlaniem RANK. Patrz debug wyżej po szczegóły, jak działa ustawianie kolorków */
#define println(FORMAT, ...) printf("%c[%d;%dm [%d]: " FORMAT "%c[%d;%dm\n",  27, (1+(rank/7))%2, 31+(6+rank)%7, rank, ##__VA_ARGS__, 27,0,37);

/* wysyłanie pakietu, skrót: wskaźnik do pakietu (0 oznacza stwórz pusty pakiet), do kogo, z jakim typem */
void sendPacket(packet_t *pkt, int destination, int tag);
void sendPacket2(packet_t* pkt, int destination, int tag);
void changeState( state_t );
void changeTallow( int );
int comparePriority(const void* a, const void* b);
#endif
