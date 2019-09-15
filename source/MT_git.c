#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define DIMBUFFER 100
#define STRINGBUFFER 128
#define DIMNASTRO 128 
#define DIMREAD 96
#define DIMSTATE 64

typedef struct end_trans
{
    unsigned int end_state;
    char move;
    char write;
    struct end_trans *next;
}End_Trans; // 16 byte

typedef struct start_trans
{
    End_Trans *read[DIMREAD];  
}Start_Trans; // 768 byte

typedef struct computation
{
    unsigned int dimleft;
    unsigned int dimright;
    char *left;
    char *right;
    unsigned int state;
    long int step;
    unsigned int poshead;
    char current;
    struct computation *next;
}Computation;

typedef struct MT
{
    Start_Trans *transitions;
    unsigned int actualdim;
    unsigned int max_state;
    unsigned int max_step;
    //stati di accettazione
    char *acc;
    int accdim;
}mt;

static inline End_Trans* insert_head(End_Trans *first, End_Trans *new_endtrans) //first è il contenuto di read[i]
{
    new_endtrans->next = first;
    first = new_endtrans;
    return first;
};

static inline void extendedarray(Start_Trans **array, unsigned int nextdim, unsigned int actualdim) // Inizializza a NULL
{
    *array = realloc(*array, (nextdim)*sizeof(Start_Trans)); //array è start_trans
    if(*array == NULL)
        exit(EXIT_FAILURE);
    for(unsigned int i=actualdim; i<nextdim; i++)
    {
       memset((*array)[i].read, '\0' ,DIMREAD*8);
    }

};

static inline void reducedarray(Start_Trans **array, long unsigned int max_state)
{
    *array = realloc(*array, (max_state)*sizeof(Start_Trans));
    if(*array == NULL)
        exit(EXIT_FAILURE);
};

/**
 * Parsa la funzione di transizione della macchina di Turing
 */
static inline mt* mt_function()
{
    mt *machine = (mt*) malloc(sizeof(mt));
    if (machine == NULL)
        exit(EXIT_FAILURE);
    
    Start_Trans *State = NULL; // Array di indirizzamento
    unsigned int actualdim = DIMSTATE; //Dimensione attuale state
    unsigned int max_state = 0; //Contatore per ridimensionare State
    unsigned int nextdim; // Variabile d'appoggio per estendere e inizializzare State
    End_Trans *new_endtrans = NULL; // Elemento per lista transiozione

    //Variabili temporanee per parsing input
    unsigned int tmpstart, tmpend, tmpacc; 
    char tmpread, tmpwrite, tmpmove, buffer[DIMBUFFER];
    char *acc = NULL; //Vettore di char usati come booleani
    int accdim = 0;
    int max_step;


    State = (Start_Trans*) malloc(actualdim*sizeof(Start_Trans)); //Array di indirizzamento
    if (State == NULL)
        exit(EXIT_FAILURE);

    for(unsigned int i=0; i<actualdim; i++)
    {
       for(int j=0; j<96; j++)
        {
            (State)[i].read[j] = NULL;
        }
    }
    fgets(buffer, DIMBUFFER, stdin); //Consuma tr
// CICLO ACQUISIZIONE FUNZIONE DI TRANSIZIONE
    while(scanf("%u %c %c %c %u", &tmpstart, &tmpread, &tmpwrite, &tmpmove, &tmpend) == 5)
    {
        if(tmpstart >= actualdim) // Incrementa il vettore a multipli di 64 celle
        {
            nextdim = actualdim;
            while(tmpstart >= nextdim)
                nextdim += DIMSTATE;
            extendedarray(&State, nextdim, actualdim);
            actualdim = nextdim;
        }
        new_endtrans = malloc(sizeof(End_Trans));
        if(new_endtrans == NULL)
            exit(EXIT_FAILURE);
        new_endtrans->end_state = tmpend;
        new_endtrans->move = tmpmove;
        new_endtrans->write = tmpwrite;
        new_endtrans->next = NULL;
        ((State[tmpstart]).read[tmpread-32]) = insert_head( ((State[tmpstart]).read[tmpread-32]), new_endtrans);
        if(tmpstart > max_state)
            max_state = tmpstart;
    }
    reducedarray(&State, max_state+1);
    actualdim = max_state+1;
// FINE CICLO, STATE COMPLETAMENTE PIENO DI DIMENSIONE ACTUALDIM
    if(fgets(buffer, DIMBUFFER, stdin)); // consuma ACC
    acc = malloc((actualdim)*sizeof(char));
    memset(acc, 'F',sizeof(char)*(actualdim));
    accdim = actualdim;
// CICLO ACQUISIZIONE STATI DI ACCETTAZIONE
    int cont = 0;
    while(scanf(" %u\n", &tmpacc) == 1)
    {
        if(tmpacc >= accdim)
        {
            acc = realloc(acc, (tmpacc+64)*sizeof(char));
            for(unsigned int k = accdim; k<tmpacc+64; k++)
                acc[k] = 'F';
            acc[tmpacc] = 'T';
            accdim = tmpacc+64;
        }
        else
        {
            acc[tmpacc] = 'T';
        }
    }
// FINE CICLO, ACC COMPLETAMENTE PIENO, DIMESIONE ACCDIM
    if(fgets(buffer, DIMBUFFER, stdin)); // consuma MAX
    scanf(" %u\n", &max_step);
    if(fgets(buffer, DIMBUFFER, stdin)); // consuma run

    machine->transitions = State;
    machine->max_state = max_state;
    machine->max_step = max_step;
    machine->actualdim = actualdim;
    machine->acc = acc;
    machine->accdim = accdim;

    return machine;
}
static inline Computation *new_Computation(Computation *old_computation, unsigned int end_state, char move, char write)
{
    unsigned int dim;
    old_computation->step++;
    old_computation->state = end_state;
    old_computation->next = NULL;
    if(old_computation->current == 'r')
        old_computation->right[old_computation->poshead] = write;
    else
        old_computation->left[old_computation->poshead] = write;
    if(move == 'L')
    {
        if(old_computation->current == 'r')
        {
            if(old_computation->poshead > 0)
                old_computation->poshead--;
            else
            {
                old_computation->poshead = 0;
                old_computation->current = 'l';
            }
        }
        else if(old_computation->current == 'l')// sono nel nastro a sinistra
        {
            if(old_computation->poshead < (old_computation->dimleft)-1) //se ho spazio
                old_computation->poshead++;
            else
            {
                dim = old_computation->dimleft;
                old_computation->dimleft = old_computation->dimleft + DIMNASTRO;
                old_computation->left = realloc(old_computation->left, sizeof(char)*(old_computation->dimleft));
                for(long unsigned int i = dim; i < old_computation->dimleft; i++)
                    old_computation->left[i] = '_';
                old_computation->poshead++;
            }
        }
    }
    else if(move == 'R')
    {
        if(old_computation->current == 'r')
        {
            if(old_computation->poshead < (old_computation->dimright)-1)
                old_computation->poshead++;
            else
            {
                dim = old_computation->dimright;
                old_computation->dimright = old_computation->dimright + DIMNASTRO;
                old_computation->right = realloc(old_computation->right, sizeof(char)*(old_computation->dimright));
                for(long unsigned int i=dim; i<old_computation->dimright; i++)
                    old_computation->right[i] = '_';
                old_computation->poshead++;
            }
        }
        else if(old_computation->current == 'l') // sono a sinistra
        {
            if(old_computation->poshead > 0)
                old_computation->poshead--;
            else
            {
                old_computation->poshead = 0;
                old_computation->current = 'r';
            }
        }
    }
    return old_computation;
}

static inline Computation *new_ND_computation(Computation *old_computation, unsigned int end_state, char move, char write)
{
    Computation *new_computation = NULL; unsigned int dim;
    new_computation = malloc(sizeof(Computation));
    new_computation->next = NULL;
    new_computation->step = old_computation->step;
    new_computation->step++;
    new_computation->state = end_state;
    new_computation->poshead = old_computation->poshead;
    new_computation->current = old_computation->current;
    new_computation->dimright = old_computation->dimright;
    new_computation->dimleft = old_computation->dimleft;
    new_computation->right = malloc(sizeof(char)*(new_computation->dimright));
    new_computation->left = malloc(sizeof(char)*(new_computation->dimleft)); 
    memcpy(new_computation->right, old_computation->right, sizeof(char)*(new_computation->dimright));
    memcpy(new_computation->left, old_computation->left, sizeof(char)*(new_computation->dimleft));

    if(new_computation->current == 'r')
        new_computation->right[new_computation->poshead] = write;
    else if(new_computation->current == 'l')
        new_computation->left[new_computation->poshead] = write;
    if(move == 'L')
    {
        if(new_computation->current == 'r')
        {
            if(new_computation->poshead > 0)
                new_computation->poshead--;
            else
            {
                new_computation->poshead = 0;
                new_computation->current = 'l';
            }
        }
        else if(new_computation->current == 'l')// sono nel nastro a sinistra
        {
            if(new_computation->poshead < (new_computation->dimleft)-1) //se ho spazio
                new_computation->poshead++;
            else
            {
                dim = new_computation->dimleft;
                new_computation->dimleft = new_computation->dimleft + DIMNASTRO;
                new_computation->left = realloc(new_computation->left, sizeof(char)*(new_computation->dimleft));
                for(unsigned int i=dim; i < new_computation->dimleft; i++)
                    new_computation->left[i] = '_';
                new_computation->poshead++;
            }
        }
    }
    else if(move == 'R')
    {
        if(new_computation->current == 'r')
        {
            if(new_computation->poshead < (new_computation->dimright)-1)
                new_computation->poshead++;
            else
            {
                dim = new_computation->dimright;
                new_computation->dimright = new_computation->dimright + DIMNASTRO;
                new_computation->right = realloc(new_computation->right, sizeof(char)*(new_computation->dimright));
                for(unsigned int i=dim; i< new_computation->dimright;  i++)
                    new_computation->right[i] = '_';
                new_computation->poshead++;
            }
        }
        else // sono a sinistra
        {
            if(new_computation->poshead > 0)
                new_computation->poshead--;
            else
            {
                new_computation->poshead = 0;
                new_computation->current = 'r';
            }
        }
    }
    return new_computation;  
}
static inline void clear_mem(mt *machine)
{
    End_Trans *iter = NULL, *next = NULL; 

    if (machine == NULL)
        return;
    for (int i = 0; i < machine->actualdim; i++)
    {
        for (int j = 0; j < DIMREAD; j++)
        {
            if (machine->transitions[i].read[j] != NULL)
            {
                iter = machine->transitions[i].read[j];
                while (iter != NULL)
                {
                    next = iter->next;
                    free(iter);
                    iter = next;
                } 
            } 
        }  
    }
    free(machine->acc);
    free(machine->transitions);
    free(machine);
}

static inline void print_mt(mt *machine)
{
    int y=0;
    End_Trans *look = NULL; 
   
    printf("actual dim %u\n", machine->actualdim);
    for(int i = 0; i < machine->actualdim; i++)
    {
        for(int j = 0; j < DIMREAD; j++)
        {
            if(machine->transitions[i].read[j] != NULL)
            {
                look = machine->transitions[i].read[j];
                while(look != NULL)
                {
                    printf("%d %c ", i, (j+32));
                    printf("%c %c %d \n", look->write, look->move, look->end_state);
                    y += 1;
                    look = look->next;
                }
            }
        }
    }
    printf("x= %d\n", y);
    printf("acc dim %u\n",machine->accdim);
    
    for(long unsigned int i = 0; i < machine->accdim; i++)
    {
        if(machine->acc[i] == 'T')
            printf("%lu\n",i);    
    }
    printf("acc dim = %u", machine->accdim);
    printf("\n");
    printf("max = %u\n", machine->max_step);
}

static inline void clear_comp(Computation *list)
{
    Computation *next = NULL;
    while (list != NULL)
    {
        next = list->next;
        free(list->right);
        free(list->left);
        free(list);
        list = next;
    }    
}

int main(int argc, char const *argv[])
{
    Computation *curr_comp = NULL;
    Computation *head = NULL;
    Computation *new_list_comp = NULL; 
    Computation *tmp_comp = NULL;
    mt *machine = mt_function();
    int accept = 0, undet = 0;
    End_Trans *iter = NULL;
    unsigned int i = 0;
    //print_mt(machine);
    curr_comp = malloc(sizeof(Computation));
    curr_comp->next = NULL;
    curr_comp->state = 0;
    curr_comp->step = 0;
    curr_comp->poshead = 0;
    curr_comp->current = 'r';
    curr_comp->dimleft = DIMNASTRO;
    curr_comp->dimright = DIMNASTRO;
    curr_comp->right = NULL;
    curr_comp->left = malloc(sizeof(char)*(curr_comp->dimleft));
    memset(curr_comp->left, '_', sizeof(char)*curr_comp->dimleft);
    scanf(" %ms", &curr_comp->right);    
    int k = 0;
    while(curr_comp->right[k] != '\0')
        k++;
    curr_comp->right[k] = '_';
    curr_comp->dimright = k+1;
    
    head = curr_comp;
    while (1)
    {
        iter = NULL;
        while (head != NULL && accept != 1)
        {
            curr_comp = head;
            while (curr_comp != NULL && accept != 1)
            {
                if(curr_comp->current == 'r')
                    i = (curr_comp->right[curr_comp->poshead]) -32; // i è l'indice di read[]
                else if(curr_comp->current == 'l')
                    i = (curr_comp->left[curr_comp->poshead]) -32;
                if(machine->actualdim > curr_comp->state)// cosi non sfondo il vett.
                {
                    iter = machine->transitions[curr_comp->state].read[i]; //non so se sia giusto
                }
                if(curr_comp->state < machine->accdim && machine->acc[curr_comp->state] == 'T') //accetto
                {
                    accept = 1;
                    undet = 0;
                    head = curr_comp;
                    clear_comp(head);
                    head = new_list_comp;
                    clear_comp(head);
                    head = NULL;
                    new_list_comp = NULL;
                    break;
                }
                else if (curr_comp->step == machine->max_step) //U
                {
                    head = curr_comp->next;
                    free(curr_comp->left);
                    free(curr_comp->right);
                    free(curr_comp);
                    undet = 1;
                    curr_comp = head;   
                }
                else if (iter == NULL) //stato pozzo
                {
                    head = curr_comp->next;
                    free(curr_comp->left);
                    free(curr_comp->right);
                    free(curr_comp);
                    curr_comp = head;
                }
                else if (iter != NULL)
                {
                    while(iter->next != NULL) // esce dal ciclo con una computazione ancora da seguire
                    {                     
                        tmp_comp = new_ND_computation(curr_comp,iter->end_state, iter->move, iter->write);
                        tmp_comp->next = new_list_comp;
                        new_list_comp = tmp_comp;
                        iter = iter->next;
                    } // ho ancora un elemento nella lista di adiacenza
                    head = curr_comp->next;                                       
                    curr_comp = new_Computation(curr_comp, iter->end_state, iter->move, iter->write);
                    curr_comp->next = new_list_comp;
                    new_list_comp = curr_comp;
                    curr_comp = head;  
                    iter = NULL;  
                }
            } 
            head = new_list_comp;
            new_list_comp = NULL;
            iter = NULL;  
        }//esco da questo ciclo se non ho più computazioni eseguibili oppure ho accettato, TEST STRINGA FINITO
        if(accept == 1)
            printf("1\n");
        else if(undet == 1)
            printf("U\n");
        else printf("0\n");
        head = NULL;
        curr_comp = malloc(sizeof(Computation));
        curr_comp->state = 0;
        curr_comp->step = 0;
        curr_comp->poshead = 0;
        curr_comp->next = NULL;
        curr_comp->current = 'r';   
        curr_comp->dimleft = DIMNASTRO;
        curr_comp->left = malloc(sizeof(char)*(curr_comp->dimleft));
        memset(curr_comp->left, '_', DIMNASTRO);
        if(scanf(" %ms", &curr_comp->right)==1)
        {
            k = 0;
            while(curr_comp->right[k] != '\0')
                k++;
            curr_comp->right[k] = '_';
            curr_comp->dimright = k+1;
        }
        else
            break;
        accept = 0; undet = 0; 
        head = curr_comp;
    }
    clear_mem(machine);
    free(curr_comp->right);
    free(curr_comp->left);
    free(curr_comp);
    return 0;
}
