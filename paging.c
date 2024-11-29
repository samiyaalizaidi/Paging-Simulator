#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <string.h>
#include <stdint.h>

/*DEFINING DATA STRUCTURES*/

// PageTable structure with entries that map logical pages to physical frames.
typedef struct PageTable {
    int frame_number; // frame assigned
    int valid;        // valid or not?  
} PageTable;

// PCB to be used whenever a new process is created.
typedef struct PCB {
    uint8_t pid;           // naam hi kafi hai
    int proc_size;         // code size + data size
    char* filename;        // name of file
    PageTable* PTE;        // page table for this process
    uint16_t code_size;    // 2 bytes
    uint8_t* code_segment; // 1 byte at a time
    uint16_t data_size;    // 2 bytes
    uint8_t* data_segment; // 1 byte at a time
    int num_pages;         // number of pages for this process
} PCB;

/*GLOBAL VARIABLES*/

int physical_mem_size;          // size of physical memory
int logical_add_size;           // size of the logical address space
int page_size;                  // size of each page 
int num_procs;                  // number of processes in the memory
int num_frames;                 // number of frames
int internal_fragmentation = 0; // to keep track of fragmentation
char** proc_list;               // list of processes
PCB** pcb_list;                 // list of PCBs
int* free_frames;               // list of free frames; -1: no frame, 0: not free, 1: free frame
int** frame_list;               // to store data of frames

/*FUNCTIONS*/

// initialize the list of free frames
void free_frames_init() {
    free_frames = (int*) malloc(num_frames * sizeof(int));
    if (!free_frames) {
        fprintf(stderr, "Couldnt allocate memory for the free frames.\n");
        return;
    }
    for (int i = 0; i < num_frames; i++) {
        free_frames[i] = 1; // frame is free
    }

    frame_list = malloc(num_frames * sizeof(int*));
}

// to find a frame for a process
int get_frame() {
    for (int i = 0; i < num_frames; i++) {
        if (free_frames[i] == 1) {
            free_frames[i] = 0; // change to not free
            return i;
        }
    }
    return -1; // free frame not found
}

// free the frame
void release_frame(int frame_number) {
    if (frame_number >= 0 && frame_number < num_frames) {
        free_frames[frame_number] = 1; // free now
    }
}

// parses the binary file to instantiate a PCB for a process. 
// also copies data into the assigned PFNs
PCB* parse_bin_file(char* filename, int page_size) {

    FILE*  file = fopen(filename, "rb");
    if (!file) { // guard
        fprintf(stderr, "Couldn't open file: %s\n", filename);
        return NULL;
    }

    PCB* pcb = malloc(sizeof(PCB));
    if (!pcb) { // guard
        fprintf(stderr, "Couldn't allocate memory for PCB\n");
        fclose(file);
        return NULL;
    }

    // get filename
    pcb->filename = malloc(strlen(filename) + 1);
    if (!pcb->filename) { // guard
        fprintf(stderr, "Couldn't allocate memory for filename\n");
        free(pcb);
        fclose(file);
        return NULL;
    }

    strcpy(pcb->filename, filename);

    /*
        Referred to this discussion for reading binary files:
        https://stackoverflow.com/questions/17598572/how-to-read-write-a-binary-file
    */

    // for pid -> 1 byte
    if (fread(&pcb->pid, sizeof(uint8_t), 1, file) != 1) {
        fprintf(stderr, "Coudln't read process ID\n");
        free(pcb->filename);
        free(pcb);
        fclose(file);
        return NULL;
    }

    // for code size -> 2 bytes
    if (fread(&pcb->code_size, sizeof(uint16_t), 1, file) != 1) {
        fprintf(stderr, "Couldn't read code size\n");
        free(pcb->filename);
        free(pcb);
        fclose(file);
        return NULL;
    }
    pcb->code_size = pcb->code_size >> 8; // jugaar? idk why this works
    printf("Code size read: %u\n", pcb->code_size);

    // read code segmemt
    pcb->code_segment = malloc(pcb->code_size);
    if (!pcb->code_segment) { // guard
        fprintf(stderr,"Couldnt allocate memory for code segment\n");
        free(pcb->filename);
        free(pcb);
        fclose(file);
        return NULL;
    }
    if (fread(pcb->code_segment, 1, pcb->code_size, file) != pcb->code_size) {
        fprintf(stderr,"Couldn't read code segment\n");
        free(pcb->code_segment);
        free(pcb->filename);
        free(pcb);
        fclose(file);
        return NULL;
    }

    // for data size -> 2 bytes
    if (fread(&pcb->data_size, 2, 1, file) != 1) {
        fprintf(stderr, "Couldn't read data size\n");
        free(pcb->code_segment);
        free(pcb->filename);
        free(pcb);
        fclose(file);
        return NULL;
    }
    pcb->data_size = pcb->data_size >> 8;
    printf("Data size read: %u\n", pcb->data_size);

    // read data segment
    pcb->data_segment = malloc(pcb->data_size);
    if (!pcb->data_segment) { // guard
        fprintf(stderr, "Couldnt allocate memory for data segment\n");
        free(pcb->code_segment);
        free(pcb->filename);
        free(pcb);
        fclose(file);
        return NULL;
    }
    if (fread(pcb->data_segment, 1, pcb->data_size, file) != pcb->data_size) {
        fprintf(stderr, "Error reading data segment\n");
        free(pcb->data_segment);
        free(pcb->code_segment);
        free(pcb->filename);
        free(pcb);
        fclose(file);
        return NULL;
    }

    // for end of file marker
    uint8_t end_marker;
    if (fread(&end_marker, sizeof(uint8_t), 1, file) != 1 || end_marker != 0xFF) {
        fprintf(stderr, "Invalid end marker in file\n");
        free(pcb->data_segment);
        free(pcb->code_segment);
        free(pcb->filename);
        free(pcb);
        fclose(file);
        return NULL;
    }

    // calculate proc size
    pcb->proc_size = pcb->code_size + pcb->data_size;

    // calculate number of pages required
    // value truncates and gives incorrect answer if ceil not used.
    pcb->num_pages = (int) ceil((double) pcb->proc_size / page_size);

    // create the page table entry
    pcb->PTE = malloc(pcb->num_pages * sizeof(PageTable));

    if (!pcb->PTE) { // guard
        fprintf(stderr, "Couldnt allocate memory for page table\n");
        free(pcb->data_segment);
        free(pcb->code_segment);
        free(pcb->filename);
        free(pcb);
        fclose(file);
        return NULL;
    }

    int code = 0;
    int count = 0;

    // set the PTE values
    for (int i = 0; i < pcb->num_pages; i++) {
        pcb->PTE[i].frame_number = get_frame(); // get frame number for this process
        pcb->PTE[i].valid = 1;                  // set valid bit

        // update internal fragmentaiton
        if (i == pcb->num_pages - 1) {
            int used_space = pcb->proc_size % page_size;
            if (used_space != 0) {
                internal_fragmentation += page_size - used_space;
            }
        }
        /*
        In paging, once the frames have been assigned, the data should be
        copied into the respective frames. 
        This is what will happen in the following block of the code.
        First we will add code segment. And then data segment. Both will be added contiguously.
        */

        frame_list[pcb->PTE[i].frame_number] = malloc(page_size * sizeof(int));
        for (int k = 0; k < page_size; k++) {
            if (count > pcb->proc_size){ // when there's no more data
                frame_list[pcb->PTE[i].frame_number][k] = 0;
                 continue;
            } 

            if (code < pcb->code_size) { // when code segment is not completely copied
                frame_list[pcb->PTE[i].frame_number][k] = pcb->code_segment[(i * page_size) + k];
                code++; count++;
            }

            else { // code segment completely copied. time for data segment
                frame_list[pcb->PTE[i].frame_number][k] = pcb->data_segment[count - code];
                count++;
            }
        }
    }

    fclose(file);
    return pcb;
}

// releases memory of PCB
void free_pcb(PCB* pcb) {
    if (pcb) {
        free(frame_list[pcb->PTE->frame_number]);
        release_frame(pcb->PTE->frame_number);
        free(pcb->PTE);
        free(pcb->data_segment);
        free(pcb->code_segment);
        free(pcb->filename);
        free(pcb);
    }
}

// stores a PCB in the PCB list
void store_PCB() {
    pcb_list = malloc(num_procs * sizeof(PCB*));

    for (int i = 0; i < num_procs; i++) {
        pcb_list[i] = parse_bin_file(proc_list[i], page_size);

        if (!pcb_list[i]) { // guard
            fprintf(stderr, "Couldn't parse file: %s\n", proc_list[i]);
            for (int j = 0; j < i; j++) {
                free_pcb(pcb_list[j]);
            }
            free(pcb_list);
            return;
        }

        printf("Process %d (%s),", i + 1, pcb_list[i]->filename);
        printf("  PID: %d,", pcb_list[i]->pid);
        printf("  Process Size: %d bytes,", pcb_list[i]->proc_size);
        printf("  Number of Pages: %d\n", pcb_list[i]->num_pages);
    }
}

// displays the page -> frame mapping for each proc
// displays the contents within the PFNs
void display_memory_dump(PCB** pcb_list, int num_procs) {
    printf("\nMemory Dump (Page -> Frame Mapping):\n");
    
    printf("\nTotal Internal Fragmentation: %d bytes\n\n", internal_fragmentation);

    // iteratre through processes
    for (int i = 0; i < num_procs; i++) {
        printf("Process %d (%s):\n", pcb_list[i]->pid, pcb_list[i]->filename);

        // display page table
        printf("  Page Table:\n");
        for (int j = 0; j < pcb_list[i]->num_pages; j++) {

            // check for validity first
            if (pcb_list[i]->PTE[j].valid) {
                printf("    Page %d -> Frame %d\n", j, pcb_list[i]->PTE[j].frame_number);
            } else {
                printf("    Page %d -> Invalid\n", j);
            }
        }

        // page wise data display
        printf("  Page/Frame-wise Data:\n");

        for (int j = 0; j < pcb_list[i]->num_pages; j++) {
            int x = pcb_list[i]->PTE[j].frame_number; // get the PFN assigned to this page
            printf("    Page %d (Frame %d): ", j, x);

            // find length for loop
            // printf("j: %d, code: %d, page size: %d\n",j,  pcb_list[i]->code_size , page_size);

            // display the contents within the frame
            for (int k = 0; k < page_size; k++) {
                printf("%02X ", frame_list[x][k]);
            }

            printf("\n\n");
        }
        printf("\n");
    }
}

void display_free_frames() {
    printf("Free Frames:\n");
    
    printf("FRAMES - ");
    for (int i = 0; i < num_frames; i++) {
        if (free_frames[i] == 1) {
            printf("%d ", i);
        }
    }
    printf("- ARE FREE \n");
}


int main(int argc, char* argv[]) {

    // guard conditions for arguments
    if (argc < 5) {
        printf("Please enter exactly five arguments: \n"
               "<physical memory size in B> \n"
               "<logical address size in bits>\n"
               "<page size>\n"
               "<path to process 1 file> \n"
               "<path to process 2 file>..."
               "<path to process n file>\n");
        return 1;
    } 

    // inputs
    physical_mem_size = atoi(argv[1]);
    logical_add_size = atoi(argv[2]);
    page_size = atoi(argv[3]);
    num_procs = argc - 4;

    // derive the number of free frames
    num_frames = physical_mem_size / page_size;

    printf("Physical Page Size: %d\n", physical_mem_size);
    printf("Logical Address Size: %d\n", logical_add_size);
    printf("Page Size: %d\n", page_size);
    printf("Number of Frames: %d\n", num_frames);

    // to store the list of processes
    proc_list = malloc(num_procs * sizeof(char* ));   

    // ensure the memory was allocated
    if (proc_list == NULL){
        fprintf(stderr, "Could not allocate memory for process list. Exiting...\n");
        return 1;
    }

    // populate the list of processes
    for(int i = 0; i < num_procs; i++) {
        proc_list[i] = argv[i + 4];
    } 

    // for debug
    for (int i = 0; i < num_procs; i++) {
        printf("  Path %d: %s\n", i + 1, proc_list[i]);
    }

    free_frames_init(); // initialize the frames list

    store_PCB(); // create a list of PCBs

    display_memory_dump(pcb_list, num_procs); // main memory dump

    display_free_frames();  // printint the free frame list

    /* RELEASE ALL DYNAMICALLY ALLOCATED MEMORY */
    
    // PCBs
    for (int i = 0; i < num_procs; i++) {
        free_pcb(pcb_list[i]);
    }

    free(pcb_list);    // delete the big list 
    free(proc_list);   // release memory allocated for process list
    free(free_frames); // release the frames
    free(frame_list);  // release the frame memory

    return 0;

}