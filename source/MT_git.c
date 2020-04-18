#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#define BUFFER_SIZE 100
#define CHUNK_TAPE 128
#define READABLE_CHAR 96
#define CHUNK_STATE 64
#define OFFSET_ASCII 32

typedef struct end_trans
{
    uint32_t end_state;
    char move;
    char write;
    struct end_trans *next;
}end_trans_t;

typedef struct start_trans
{
    end_trans_t *read[READABLE_CHAR];
}start_trans_t;

typedef struct computation
{
    uint32_t left_tape_size;
    uint32_t right_tape_size;
    char *left_tape;
    char *right_tape;
    uint32_t state;
    uint64_t step;
    uint32_t pos_head;
    char current_tape;
    struct computation *next;
}computation_t;

typedef struct MT
{
    start_trans_t *transitions;
    uint32_t states_array_size;
    uint32_t max_step;
    char *acceptance_states;
    uint32_t acceptance_states_size;
}turing_machine_t;

static inline end_trans_t* insert_head(end_trans_t *first, end_trans_t *new_end_trans);

static inline void extend_array_states(start_trans_t **states_array, const uint32_t new_states_array_size, uint32_t *states_array_size);

static inline void reduce_array_states(start_trans_t **array, uint32_t *states_array_size, const uint32_t max_state);

static inline start_trans_t *init_start_trans();

static inline computation_t *init_computation();

static inline computation_t *init_new_computation(const computation_t *old_computation);

static inline void check_expected_input(const char *input_string);

static inline void read_acceptance_states(const uint32_t states_array_size, uint32_t *acceptance_states_size, char **acceptance_states);

static inline void read_transition_function(uint32_t *states_array_size, start_trans_t **states_array);

static inline turing_machine_t* read_turing_machine();

static inline computation_t *perform_computation(computation_t *computation, const uint32_t end_state, const char move, const char write);

static inline computation_t *perform_ND_computation(const computation_t *old_computation, uint32_t end_state, char move, char write);

static inline void free_turing_machine(turing_machine_t *machine);

static inline void free_computation(computation_t *computation);

static inline void free_computations_list(computation_t *list);

static inline bool read_input_string(computation_t *current_computation);

static inline void print_mt(turing_machine_t *machine);

int main(int argc, char const *argv[])
{
    bool is_accept = false, is_not_determinable = false;
    uint32_t char_to_index = 0;
    computation_t *head = NULL;
    computation_t *new_list_comp = NULL;
    computation_t *tmp_comp = NULL;
    end_trans_t *iterator = NULL;
    turing_machine_t *machine = read_turing_machine();
    computation_t *current_computation = init_computation();
    read_input_string(current_computation);
    head = current_computation;
    /**
     * Main cycle, reads input string and starts calculation.
     * Cycle ends when there are no strings to read.
     */
    while (true)
    {
        /**
         * Test string cycle, performs BFS on computation graph with two lists.
         * The cycle ends when a computation accepts the string or
         * moves are no longer available.
        **/
        iterator = NULL;
        while (head != NULL && !is_accept)
        {
            current_computation = head;
            while (current_computation != NULL)
            {
                if(current_computation->current_tape == 'r')
                    char_to_index = (current_computation->right_tape[current_computation->pos_head]) - OFFSET_ASCII; // char_to_index è l'indice di read[]
                else if(current_computation->current_tape == 'l')
                    char_to_index = (current_computation->left_tape[current_computation->pos_head]) - OFFSET_ASCII;
                if(current_computation->state < machine->states_array_size)
                {
                    iterator = machine->transitions[current_computation->state].read[char_to_index];
                }
                // Acceptance state, end string test
                if(current_computation->state < machine->acceptance_states_size && machine->acceptance_states[current_computation->state] == 'T')
                {
                    is_accept = true;
                    is_not_determinable = false;
                    free_computations_list(current_computation);
                    free_computations_list(new_list_comp);
                    new_list_comp = NULL;
                    break;
                }
                    // Undetermined
                else if (current_computation->step == machine->max_step)
                {
                    head = current_computation->next;
                    free_computation(current_computation);
                    current_computation = head;
                    is_not_determinable = true;
                }
                    // Sink state
                else if (iterator == NULL)
                {
                    head = current_computation->next;
                    free_computation(current_computation);
                    current_computation = head;
                }
                else if (iterator != NULL)
                {
                    // Non deterministic computation
                    while(iterator->next != NULL)
                    {
                        tmp_comp = perform_ND_computation(current_computation, iterator->end_state, iterator->move, iterator->write);
                        tmp_comp->next = new_list_comp;
                        new_list_comp = tmp_comp;
                        iterator = iterator->next;
                    }
                    // Deterministic computation
                    head = current_computation->next;
                    current_computation = perform_computation(current_computation, iterator->end_state, iterator->move, iterator->write);
                    current_computation->next = new_list_comp;
                    new_list_comp = current_computation;
                    current_computation = head;
                    iterator = NULL;
                }
            }
            head = new_list_comp;
            new_list_comp = NULL;
            iterator = NULL;
        }
        if(is_accept)
            printf("1\n");
        else if(is_not_determinable)
            printf("U\n");
        else printf("0\n");
        current_computation = init_computation();
        if(!read_input_string(current_computation))
            break;
        is_accept = false;
        is_not_determinable = false;
        head = current_computation;
    }
    free_turing_machine(machine);
    free_computation(current_computation);
    return 0;
}

static inline end_trans_t* insert_head(end_trans_t *first, end_trans_t *new_end_trans)
{
    new_end_trans->next = first;
    first = new_end_trans;
    return first;
}

static inline void extend_array_states(start_trans_t **states_array, const uint32_t new_states_array_size, uint32_t *states_array_size)
{
    *states_array = realloc(*states_array, (new_states_array_size) * sizeof(start_trans_t)); //states_array è start_trans
    if(*states_array == NULL)
        exit(EXIT_FAILURE);
    for(uint32_t i = *states_array_size; i < new_states_array_size; i++)
    {
        memset((*states_array)[i].read, '\0' , READABLE_CHAR * 8);
    }
    *states_array_size = new_states_array_size;
}

static inline void reduce_array_states(start_trans_t **array, uint32_t *states_array_size, const uint32_t max_state)
{
    *array = realloc(*array, (max_state)*sizeof(start_trans_t));
    if(*array == NULL)
        exit(EXIT_FAILURE);
    *states_array_size = max_state;
}

static inline start_trans_t *init_start_trans()
{
    start_trans_t *states = (start_trans_t*) malloc(CHUNK_STATE * sizeof(start_trans_t));
    if (states == NULL)
    {
        exit(EXIT_FAILURE);
    }
    for (uint32_t state_index = 0; state_index < CHUNK_STATE; state_index++)
    {
        for (uint32_t char_index = 0; char_index < READABLE_CHAR; char_index++)
        {
            states[state_index].read[char_index] = NULL;
        }
    }
    return states;
}

static inline computation_t *init_computation()
{
    computation_t *current_computation = NULL;
    current_computation = (computation_t*) malloc(sizeof(computation_t));
    if (current_computation == NULL)
        exit(EXIT_FAILURE);
    current_computation->next = NULL;
    current_computation->state = 0;
    current_computation->step = 0;
    current_computation->pos_head = 0;
    current_computation->current_tape = 'r';
    current_computation->left_tape_size = CHUNK_TAPE;
    current_computation->right_tape_size = CHUNK_TAPE;
    current_computation->right_tape = NULL;
    current_computation->left_tape = (char*) malloc(sizeof(char) * (current_computation->left_tape_size));
    if(current_computation->left_tape == NULL)
        exit(EXIT_FAILURE);
    memset(current_computation->left_tape, '_', sizeof(char) * current_computation->left_tape_size);
    return current_computation;
}

static inline computation_t *init_new_computation(const computation_t *old_computation)
{
    computation_t *new_computation = NULL;
    new_computation = (computation_t*) malloc(sizeof(computation_t));
    new_computation->next = NULL;
    new_computation->step = old_computation->step;
    new_computation->pos_head = old_computation->pos_head;
    new_computation->current_tape = old_computation->current_tape;
    new_computation->right_tape_size = old_computation->right_tape_size;
    new_computation->left_tape_size = old_computation->left_tape_size;
    new_computation->right_tape = (char*) malloc(sizeof(char) * (new_computation->right_tape_size));
    new_computation->left_tape = (char*) malloc(sizeof(char) * (new_computation->left_tape_size));
    memcpy(new_computation->right_tape, old_computation->right_tape, sizeof(char) * (new_computation->right_tape_size));
    memcpy(new_computation->left_tape, old_computation->left_tape, sizeof(char) * (new_computation->left_tape_size));
    return new_computation;
}

static inline void check_expected_input(const char *input_string)
{
    char buffer[BUFFER_SIZE];
    if(fgets(buffer, BUFFER_SIZE, stdin) == NULL)
        exit(EXIT_FAILURE);
    if (strcmp(buffer, input_string) != 0)
    {
        exit(EXIT_FAILURE);
    }
}

static inline void read_acceptance_states(const uint32_t states_array_size, uint32_t *acceptance_states_size, char **acceptance_states)
{
    uint32_t temp_acc = 0;
    *acceptance_states = (char*) malloc((states_array_size) * sizeof(char));
    memset(*acceptance_states, 'F',sizeof(char)*(states_array_size));
    *acceptance_states_size = states_array_size;
    while(scanf(" %u\n", &temp_acc) == 1)
    {
        if(temp_acc >= *acceptance_states_size)
        {
            *acceptance_states = realloc(*acceptance_states, (temp_acc + CHUNK_STATE)*sizeof(char));
            for(uint32_t k = *acceptance_states_size; k< temp_acc + CHUNK_STATE; k++)
                (*acceptance_states)[k] = 'F';

            (*acceptance_states)[temp_acc] = 'T';
            *acceptance_states_size = temp_acc + CHUNK_STATE;
        }
        else
        {
            (*acceptance_states)[temp_acc] = 'T';
        }
    }
}

static inline void read_transition_function(uint32_t *states_array_size, start_trans_t **states_array)
{
    char temp_read, temp_write, temp_move;
    uint32_t new_states_array_size, temp_start, temp_end_state, max_state = 0;
    end_trans_t *new_end_trans = NULL;
    while(scanf("%u %c %c %c %u", &temp_start, &temp_read, &temp_write, &temp_move, &temp_end_state) == 5)
    {
        if(temp_start >= *states_array_size)
        {
            new_states_array_size = *states_array_size;
            while(temp_start >= new_states_array_size)
                new_states_array_size += CHUNK_STATE;
            extend_array_states(states_array, new_states_array_size, states_array_size);
        }
        new_end_trans = (end_trans_t*) malloc(sizeof(end_trans_t));
        if(new_end_trans == NULL)
            exit(EXIT_FAILURE);
        new_end_trans->end_state = temp_end_state;
        new_end_trans->move = temp_move;
        new_end_trans->write = temp_write;
        new_end_trans->next = NULL;
        (*states_array)[temp_start].read[temp_read - OFFSET_ASCII] = insert_head((*states_array)[temp_start].read[temp_read - OFFSET_ASCII], new_end_trans);
        if(temp_start > max_state)
            max_state = temp_start;
    }
    reduce_array_states(states_array, states_array_size, max_state + 1);
}

static inline turing_machine_t* read_turing_machine()
{
    char *acceptance_states = NULL;
    uint32_t max_step = 0;
    uint32_t acceptance_states_size = 0;
    uint32_t num_states = CHUNK_STATE;
    start_trans_t *states_array = NULL;

    turing_machine_t *machine = (turing_machine_t*) malloc(sizeof(turing_machine_t));
    if (machine == NULL)
        exit(EXIT_FAILURE);

    states_array = init_start_trans();
    check_expected_input("tr\n");
    read_transition_function(&num_states, &states_array);
    check_expected_input("acc\n");
    read_acceptance_states(num_states, &acceptance_states_size, &acceptance_states);
    check_expected_input("max\n");

    if(scanf(" %u\n", &max_step) != 1)
        exit(EXIT_FAILURE);

    check_expected_input("run\n");
    machine->transitions = states_array;
    machine->max_step = max_step;
    machine->states_array_size = num_states;
    machine->acceptance_states = acceptance_states;
    machine->acceptance_states_size = acceptance_states_size;
    return machine;
}

static inline computation_t *perform_computation(computation_t *computation, const uint32_t end_state, const char move, const char write)
{
    uint32_t current_tape_size;
    computation->step++;
    computation->state = end_state;
    computation->next = NULL;
    if(computation->current_tape == 'r')
        computation->right_tape[computation->pos_head] = write;
    else
        computation->left_tape[computation->pos_head] = write;
    if(move == 'L')
    {
        if(computation->current_tape == 'r')
        {
            if(computation->pos_head > 0)
                computation->pos_head--;
            else
            {
                computation->pos_head = 0;
                computation->current_tape = 'l';
            }
        }
        else if(computation->current_tape == 'l')
        {
            if(computation->pos_head < (computation->left_tape_size) - 1)
                computation->pos_head++;
            else
            {
                current_tape_size = computation->left_tape_size;
                computation->left_tape_size = computation->left_tape_size + CHUNK_TAPE;
                computation->left_tape = realloc(computation->left_tape, sizeof(char) * (computation->left_tape_size));
                for(uint64_t i = current_tape_size; i < computation->left_tape_size; i++)
                    computation->left_tape[i] = '_';
                computation->pos_head++;
            }
        }
    }
    else if(move == 'R')
    {
        if(computation->current_tape == 'r')
        {
            if(computation->pos_head < (computation->right_tape_size) - 1)
                computation->pos_head++;
            else
            {
                current_tape_size = computation->right_tape_size;
                computation->right_tape_size = computation->right_tape_size + CHUNK_TAPE;
                computation->right_tape = realloc(computation->right_tape, sizeof(char) * (computation->right_tape_size));
                for(uint64_t i=current_tape_size; i < computation->right_tape_size; i++)
                    computation->right_tape[i] = '_';
                computation->pos_head++;
            }
        }
        else if(computation->current_tape == 'l')
        {
            if(computation->pos_head > 0)
                computation->pos_head--;
            else
            {
                computation->pos_head = 0;
                computation->current_tape = 'r';
            }
        }
    }
    return computation;
}

static inline computation_t *perform_ND_computation(const computation_t *old_computation, uint32_t end_state, char move, char write)
{
    computation_t *new_computation = init_new_computation(old_computation);
    return perform_computation(new_computation, end_state, move, write);
}

static inline void free_turing_machine(turing_machine_t *machine)
{
    end_trans_t *iterator = NULL, *next = NULL;

    if (machine == NULL)
        return;
    for (uint32_t i = 0; i < machine->states_array_size; i++)
    {
        for (uint32_t j = 0; j < READABLE_CHAR; j++)
        {
            if (machine->transitions[i].read[j] != NULL)
            {
                iterator = machine->transitions[i].read[j];
                while (iterator != NULL)
                {
                    next = iterator->next;
                    free(iterator);
                    iterator = next;
                }
            }
        }
    }
    free(machine->acceptance_states);
    free(machine->transitions);
    free(machine);
}

static inline void free_computation(computation_t *computation)
{
    free(computation->left_tape);
    free(computation->right_tape);
    free(computation);
}

static inline void free_computations_list(computation_t *list)
{
    computation_t *next = NULL;
    while (list != NULL)
    {
        next = list->next;
        free_computation(list);
        list = next;
    }
}

static inline bool read_input_string(computation_t *current_computation)
{
    if(scanf(" %ms", &current_computation->right_tape) == 1)
    {
        int k = 0;
        while(current_computation->right_tape[k] != '\0')
            k++;
        current_computation->right_tape[k] = '_';
        current_computation->right_tape_size = k+1;
        return true;
    }
    return false;
}

static inline void print_mt(turing_machine_t *machine)
{
    uint32_t y=0;
    end_trans_t *look = NULL;

    printf("actual dim %u\n", machine->states_array_size);
    for(uint32_t i = 0; i < machine->states_array_size; i++)
    {
        for(uint32_t j = 0; j < READABLE_CHAR; j++)
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
    printf("x = %d\n", y);
    printf("acc dim %u\n",machine->acceptance_states_size);

    for(uint64_t i = 0; i < machine->acceptance_states_size; i++)
    {
        if(machine->acceptance_states[i] == 'T')
            printf("%lu\n",i);
    }
    printf("acc dim = %u\n", machine->acceptance_states_size);
    printf("max = %u\n", machine->max_step);
}