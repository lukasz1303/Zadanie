#include "main.h"
#include "watek_komunikacyjny.h"
#include "watek_glowny.h"
/* wątki */
#include <pthread.h>

/* sem_init sem_destroy sem_post sem_wait */
//#include <semaphore.h>
/* flagi dla open */
//#include <fcntl.h>

state_t stan=STAN1_START;
volatile char end = FALSE;
int size,rank, shop_size; /* nie trzeba zerować, bo zmienna globalna statyczna */
MPI_Datatype MPI_PAKIET_T;
pthread_t threadKom, threadRest;
pthread_mutex_t lamportMut = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t stateMut = PTHREAD_MUTEX_INITIALIZER;

int priority = 0;
int lamport = 0;
int ack_f_counter = 0;
int ack_f_queue[100];
int ack_f_queue_cur_size = 0;
int number_of_Mediums = 2;
process* medium_request_queue;
int medium_request_queue_cur_size = 0;
medium* mediums;
int last = -1;
int last_rel = 0;
int last_rel_tun = 0;
int m_pos = -1;
int* msg_received;

void check_thread_support(int provided)
{
    printf("THREAD SUPPORT: chcemy %d. Co otrzymamy?\n", provided);
    switch (provided) {
        case MPI_THREAD_SINGLE: 
            printf("Brak wsparcia dla wątków, kończę\n");
            /* Nie ma co, trzeba wychodzić */
	    fprintf(stderr, "Brak wystarczającego wsparcia dla wątków - wychodzę!\n");
	    MPI_Finalize();
	    exit(-1);
	    break;
        case MPI_THREAD_FUNNELED: 
            printf("tylko te wątki, ktore wykonaly mpi_init_thread mogą wykonać wołania do biblioteki mpi\n");
	    break;
        case MPI_THREAD_SERIALIZED: 
            /* Potrzebne zamki wokół wywołań biblioteki MPI */
            printf("tylko jeden watek naraz może wykonać wołania do biblioteki MPI\n");
	    break;
        case MPI_THREAD_MULTIPLE: printf("Pełne wsparcie dla wątków\n"); /* tego chcemy. Wszystkie inne powodują problemy */
	    break;
        default: printf("Nikt nic nie wie\n");
    }
}

int incLamport(){
	pthread_mutex_lock( &lamportMut );
	lamport++;
	int tmp = lamport;
	pthread_mutex_unlock(&lamportMut);
	return tmp;
}

int incBiggerLamport(int n){
	pthread_mutex_lock( &lamportMut );
	lamport= (n>lamport)?n+1:lamport+1;
	int tmp = lamport;
	pthread_mutex_unlock(&lamportMut);
	return tmp;
}


/* srprawdza, czy są wątki, tworzy typ MPI_PAKIET_T
*/
void inicjuj(int *argc, char ***argv)
{
    int provided;
    shop_size = 2;
    msg_received = malloc(size * sizeof(int));
    mediums = malloc(number_of_Mediums * sizeof(medium));
    medium_request_queue = malloc((number_of_Mediums *2 + 2*size) * sizeof(medium));
    for (int i = 0; i < number_of_Mediums; i++) {
        mediums[i].tun = 3;
        mediums[i].c = 3;
    }
    MPI_Init_thread(argc, argv,MPI_THREAD_MULTIPLE, &provided);
    check_thread_support(provided);


    /* Stworzenie typu */
    /* Poniższe (aż do MPI_Type_commit) potrzebne tylko, jeżeli
       brzydzimy się czymś w rodzaju MPI_Send(&typ, sizeof(pakiet_t), MPI_BYTE....
    */
    /* sklejone z stackoverflow */
    const int nitems=3; /* bo packet_t ma trzy pola */
    int       blocklengths[3] = {1,1,1};
    MPI_Datatype typy[3] = {MPI_INT, MPI_INT, MPI_INT};

    MPI_Aint     offsets[3]; 
    offsets[0] = offsetof(packet_t, ts);
    offsets[1] = offsetof(packet_t, src);
    offsets[2] = offsetof(packet_t, data);

    MPI_Type_create_struct(nitems, blocklengths, offsets, typy, &MPI_PAKIET_T);
    MPI_Type_commit(&MPI_PAKIET_T);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    srand(rank);
  
    pthread_create( &threadKom, NULL, startKomWatek , 0);
   // if (rank==0) {
	//pthread_create( &threadMon, NULL, startMonitor, 0);
    //}
    debug("jestem");
}

/* usunięcie zamkków, czeka, aż zakończy się drugi wątek, zwalnia przydzielony typ MPI_PAKIET_T
   wywoływane w funkcji main przed końcem
*/
void finalizuj()
{
    pthread_mutex_destroy( &stateMut);
    /* Czekamy, aż wątek potomny się zakończy */
    println("czekam na wątek \"komunikacyjny\"\n" );
    pthread_join(threadKom,NULL);
    if (rank==0) pthread_join(threadMon,NULL);
    MPI_Type_free(&MPI_PAKIET_T);
    MPI_Finalize();
}


/* opis patrz main.h */
void sendPacket(packet_t *pkt, int destination, int tag)
{
    int freepkt=0;
    if (pkt==0) { pkt = malloc(sizeof(packet_t)); freepkt=1;}
    pkt->src = rank;
	int tmp = incLamport();
	pkt->ts = tmp;
    MPI_Send( pkt, 1, MPI_PAKIET_T, destination, tag, MPI_COMM_WORLD);
    if (freepkt) free(pkt);
}

void sendPacket2(packet_t* pkt, int destination, int tag)
{
    int freepkt = 0;
    if (pkt == 0) { pkt = malloc(sizeof(packet_t)); freepkt = 1; }
    pkt->src = rank;
    pkt->ts = lamport;
    MPI_Send(pkt, 1, MPI_PAKIET_T, destination, tag, MPI_COMM_WORLD);
    if (freepkt) free(pkt);
}


void changeState( state_t newState )
{
    pthread_mutex_lock( &stateMut );
    stan = newState;
    pthread_mutex_unlock( &stateMut );
}

int comparePriority(const void* a, const void* b) {

    const struct process* part1 = (struct process*)a;
    const struct process* part2 = (struct process*)b;

    const int s3 = part1->priority;
    const int s4 = part2->priority;
    return s3 > s4;
}

int main(int argc, char **argv)
{
    /* Tworzenie wątków, inicjalizacja itp */
    inicjuj(&argc,&argv); // tworzy wątek komunikacyjny w "watek_komunikacyjny.c"
     // by było wiadomo ile jest łoju
    mainLoop();          // w pliku "watek_glowny.c"

    finalizuj();
    return 0;
}

