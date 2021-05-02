#include "cachelab.h"
#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>

//initialize the components of a cache
int sets;
int assoc;
int bytes; //bytes per block

// a stuct that contains all that you need for a block in the cache
typedef struct Block{
    int tag;
    int valid_bit;
    int lru;
} block;

//stuct for set that has the blocks and the set numbers
typedef struct Set{
    unsigned int index;
    block *my_set;
} set;

//initalize pointer to set cache
set *my_cache;

//initalize counter for miss, hit, and evict
int miss_count = 0;
int hit_count = 0;
int eviction_count = 0;

//initialize lur counter
int counter = 0;

//craeted headers for functions
void modify(unsigned int address, int offset_bits, int set_bits);
void store(unsigned int address, int size);
void loadStore(unsigned int address, int offset_bits, int set_bits);
void evict(int set_index, int tag);
void readTrace(char *trace_file, int offset_bits, int set_bits);

/*
    Takes the parameters that are relevant for calculating set index 
    and tag.
    This wlll calculate the hit, misses, and evicts.
*/
void loadStore(unsigned int address, int offset_bits, int set_bits){
    //a mask of 1s and then shifting from the address to get the set_index
    //of the block you are interested in
    unsigned int set_index = (address >> (offset_bits)) & ((1 << set_bits)-1);
    //to get the tag, you shift address by the set_bits and offset-bits
    unsigned int tag = address >> (set_bits + offset_bits);
    //if the set index is larger, then it is invalid
    if(set_index > sets)
        return;
    
    //loops through the lines in the set to check
    //if it is a hit, miss, or evict
    for(int j=0; j<assoc; j++){
        //if hit, then add hit count
        if(my_cache[set_index].my_set[j].tag == tag){
            //accessed this block, then add to global counter
            counter++;
            //set the lru of block to be same as counter
            my_cache[set_index].my_set[j].lru = counter;
            //add to hit count
            hit_count++;
            printf(" hit");
            return;
            //if miss, then add to miss count
        }else if(my_cache[set_index].my_set[j].valid_bit != 1){
            //accessed this block, then add to global counter
            counter++;
            //set the lru of block to be same as counter
            my_cache[set_index].my_set[j].lru = counter;
            //add miss count
            miss_count++;
            //initialize the tag
            my_cache[set_index].my_set[j].tag = tag;
            //make it a valid block
            my_cache[set_index].my_set[j].valid_bit = 1;
            printf(" miss");
            return;
        }       
    }

    //add an evict count
    eviction_count++;
    //accessed this block, then add to global counter
    counter++;
    //all evict to replace the tag of one of the blocks
    evict(set_index, tag);
    //add to miss count
    miss_count++;
    printf(" miss evinction");
}

/*
    Take the command like parameters.
    Initializes the cache with the specified parameters
*/
void create_cache(int set_bits, int E, int offset_bits){
    //get total number of set
    sets = 1 << set_bits;
    //get number of lines per set
    assoc = E;
    //get total number of bytes
    bytes = 1 << offset_bits;

    //mallocs space for the cache
    my_cache = (set *) malloc(sets * sizeof(set));
    

    //for each set that we have created. we need to reserve space =  number of blocks
    int i, j;
    //loops through all the sets and initializes all blocks
    for(i = 0; i < sets; i++){
        my_cache[i].index = i;
        //mallocs space for sets
        my_cache[i].my_set =  (block *) malloc(assoc * sizeof(block));

        //initizalize parameters for each block
        for(j=0; j < assoc; j++){
            my_cache[i].my_set[j].tag = -1;
            my_cache[i].my_set[j].valid_bit = -1;
            my_cache[i].my_set[j].lru = -1;
        }

    }
}


int main(int argc, char **argv)
{   
    //initializes all variables that will be retrieved from 
    //command line
    int set_bits = 0;
    int associativity = 0;
    int offset_bits = 0;
    char *trace_file = NULL;

    
    char opt;
    //loops until you get all the commands
    while(1){
        //get each type
        opt = getopt(argc, argv, "vhs:E:t:b:");
        //if no more
        if(opt == -1) break;

        switch(opt){
            //save value next to it into set_bits
            case 's':
                set_bits = atoi(optarg);
                break;
            //save value next to it in assoc
            case 'E':
                associativity = atoi(optarg);
                break; 
            //save value next to it in offset_bits
            case 'b':
                offset_bits = atoi(optarg);
                break;
            //saves value next to it in trace_file
            case 't':
                trace_file = optarg;
                break;
            //otherwise break
            default:
                break;

        }
    }

    //prints what's in the command line
    printf("%d, %d, %d, %s \n", set_bits, associativity, offset_bits, trace_file);
    //inialize cache
    create_cache(set_bits, associativity, offset_bits);
    //read what is in the tracefile, to continue the process
    readTrace(trace_file, offset_bits, set_bits);
    //print the counts
    printSummary(hit_count, miss_count, eviction_count);
    return 0;
}

/*
    Takes the files that you need to trace, and
    set_bits and offset_bits you will need for the other functions.
    Reads the trace line by line and calls modify and load to
    calculated the miss, hit, and evict count.
*/
void readTrace(char *trace_file, int offset_bits, int set_bits){
    //if there is no tracefile provided
    if(trace_file == NULL)
        return;
    
    //open file
    FILE* pFile = fopen (trace_file,"r"); //open file for reading

    //initalize paramenters taken from the tarce_file
    char identifier;
    unsigned int address;
    int size;
    // Reading lines like " M 20,1" or "L 19,3"
    while(fscanf(pFile," %c %x,%d", &identifier, &address, &size)>0)
    {
        //prints the content of the line
        printf("%c %x,%d", identifier, address, size);

        //if L or S, call the load function
        //if M, call the modify function.
        //they will aid in calculated
        //miss, hit, and evict count
        switch(identifier){
            case 'L':
            case 'S':
                loadStore(address, offset_bits, set_bits);
                printf("\n");
                break;
            case 'M':
                modify(address, offset_bits, set_bits);
                printf("\n");
                break;
            default:
                break;
        }
    }
    //must close file after
    fclose(pFile); 
}

/*
    Takes the parameters that are relevant for calculating set index 
    and tag.
    This wlll calculate the hit, misses, and evicts by calling loadStore twice
*/
void modify(unsigned int address, int offset_bits, int set_bits){
    loadStore(address, offset_bits, set_bits);
    loadStore(address, offset_bits, set_bits);
}

/*
    Takes the set_index and tag.
    Replaces teh block that was first accessed 
*/
void evict(int set_index, int tag){
    //for finding the smallest lru
    int smallest = my_cache[set_index].my_set[0].lru;
    //find the block that has the smallest lru
    int smIndex = 0;
    //loops through until end to find smallest lru
    for(int i=0; i < assoc; i++){
        //if the lru of the block you are accessing is smaller
        //update the variables
        if(my_cache[set_index].my_set[i].lru < smallest){
            smallest = my_cache[set_index].my_set[i].lru;
            smIndex = i;
        }
    }
    //set the tag of that block to be of the new tag
    my_cache[set_index].my_set[smIndex].tag = tag;
    //added access counter to the lru as teh block is accessed
    my_cache[set_index].my_set[smIndex].lru = counter;
}
