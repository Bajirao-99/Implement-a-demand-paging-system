#include <iostream>
#include <unordered_map>
#include <queue>
#include <vector>
#include <random>

#include <limits.h>
#include <stdio.h>
#include <cstdlib>

using namespace std;

int current_timestamp_ = 0;
vector<int> swapspace(2048);


// Define the size of the memory and page size
const int MEMORY_SIZE = 512;
const int PAGE_SIZE = 64;

// Define the maximum number of pages
const int MAX_NUM_PAGES = MEMORY_SIZE / PAGE_SIZE;

// Define the probability distribution of the memory access pattern
const double READ_PROBABILITY = 0.8;
const double WRITE_PROBABILITY = 0.2;
const double LOCALITY_OF_REFERENCE = 0.9;

// Define the maximum number of memory access requests
const int MAX_ACCESS_REQUESTS = 10;

// Define the data structure for a memory access request
struct AccessRequest {
    int page_number;
    bool is_write;
};

// Define the page table entry structure
struct PageTableEntry {
    int frame_number;
    bool is_valid;
    bool is_dirty;
    int last_access_timestamp;
};

// Define the frame table entry structure
struct FrameTableEntry {
    int page_number;
    bool is_allocated;
};

// // Define the page replacement algorithm
// enum PageReplacementAlgorithm {
//     FIFO,
//     LRU
// };


char memory[MEMORY_SIZE];

// Define the demand paging system class
class DemandPagingSystem {

    private:
        int algorithm_;
        int page_hit_count;
        int page_miss_count;
        
        FILE * swapspace;

        PageTableEntry pageTable[MAX_NUM_PAGES];
        FrameTableEntry frameTable[MEMORY_SIZE / PAGE_SIZE];
        unordered_map<int, int> pageToFrameMap;
        queue<int> pageQueueInMainMemory;

        int allocateFrame() {
            // Find a free frame in the frame table
            for (int i = 0; i < MEMORY_SIZE / PAGE_SIZE; i++) {
                if (!frameTable[i].is_allocated) {
                    frameTable[i].is_allocated = true;
                    return i;
                }
            }
            return -1;
        }

        void printFrameTable() {
            printf("\nFrame Table : \n");
            printf(" -----------------------------------\n");
            printf("| Frame No | Page No | is_allocated |\n");
            printf("|-----------------------------------|\n");
            for(int i = 0; i < MAX_NUM_PAGES; i++) {
                // cout << pageTable[i].frame_number << ", " << pageTable[i].is_dirty << ", " << pageTable[i].is_valid << ", " << pageTable[i].last_access_timestamp << endl;
                printf("| %8d | %7d | %12s |\n", i, frameTable[i].page_number, frameTable[i].is_allocated ? "True" : "False");
            }
            printf(" ----------------------------------- \n");
        }

        void printPageTable() {
            printf("\nPage Table : \n");
            printf(" ------------------------------------------------------\n");
            printf("| Page No | Frame No | is_dirty | is_valid | Timestamp |\n");
            printf("|------------------------------------------------------|\n");
            for(int i = 0; i < MAX_NUM_PAGES; i++) {
                // cout << pageTable[i].frame_number << ", " << pageTable[i].is_dirty << ", " << pageTable[i].is_valid << ", " << pageTable[i].last_access_timestamp << endl;
                printf("| %7d | %8d | %8s | %8s | %9d | \n", i, pageTable[i].frame_number, pageTable[i].is_dirty ? "True" : "False", pageTable[i].is_valid ? "True" : "False", pageTable[i].last_access_timestamp);
            }
            printf(" ------------------------------------------------------\n");
        }

        void removePageFromMainMemory() {
            // Select a page to evict based on the page replacement algorithm
            int page_number;
            // algorithm : LRU
            if (algorithm_ != 0) {
                page_number = getPageByLRU();
            }
            // FIFO 
            else {
                page_number = pageQueueInMainMemory.front();
                pageQueueInMainMemory.pop();
                
            }

            // Update the page table entry
            int frame_number = pageTable[page_number].frame_number;
            pageTable[page_number].is_valid = false;

            // Save the page to disk if it is dirty
            if (pageTable[page_number].is_dirty) {
                savePageToSwap(page_number, frame_number);
            }

            // Free the frame in the frame table
            frameTable[frame_number].is_allocated = false;

            // Remove the page from the page-to-frame map
            pageToFrameMap.erase(page_number);
        }

    int getPageByLRU() {
        // Find the least recently used page in the page table
        int page_number = -1;
        int min_timestamp = INT_MAX;
        for (int i = 0; i < MAX_NUM_PAGES; i++) {
            if (pageTable[i].is_valid && pageTable[i].last_access_timestamp < min_timestamp) {
                page_number = i;
                min_timestamp = pageTable[i].last_access_timestamp;
            }
        }
        return page_number;
    }

    void savePageToSwap(int page_number, int frame_number) {
        // Calculate the byte offset of the page in the swap space
        int byte_offset = page_number * PAGE_SIZE;

        // Write the page to the swap space
        // fseek(swapspace, byte_offset, SEEK_SET);
        // fwrite(&memory[frame_number * PAGE_SIZE], sizeof(char), PAGE_SIZE, swapspace);

        // Mark the page as clean in the page table
        pageTable[page_number].is_dirty = false;
    }

    void loadPageFromSwap(int page_number, int frame_number, char* memory) {
        // Read the page from the swap space into the specified frame in physical memory
        // fseek(swapspace, page_number * PAGE_SIZE, SEEK_SET);
        // fread(&memory[frame_number * PAGE_SIZE], sizeof(char), PAGE_SIZE, swapspace);

        // Update the page table entry and the frame table entry
        pageTable[page_number].is_valid = true;
        pageTable[page_number].frame_number = frame_number;
        pageTable[page_number].last_access_timestamp = current_timestamp_;
        frameTable[frame_number].is_allocated = true;
        frameTable[frame_number].page_number = page_number;

        // Add the page to the page-to-frame map
        pageToFrameMap[page_number] = frame_number;
    }

    public:
        DemandPagingSystem(int algorithm) {
            // Initialize the page replacement algorithm
            // 0 - FIFO
            // Other - LRU
            algorithm_ = algorithm;
            
            page_hit_count = 0;
            page_miss_count = 0;
            // Initialize the page table and frame table
            for (int i = 0; i < MAX_NUM_PAGES; i++) {
                pageTable[i] = { -1, false, false, -1 };
            }
            for (int i = 0; i < MEMORY_SIZE / PAGE_SIZE; i++) {
                frameTable[i] = { -1, false };
            }


            swapspace = fopen("swapspace.txt", "r+");

        }

        void handleRequest(int page_number, char* memory, bool is_write, int current_timestamp) {
            
            // cout << "Request = { pgno : " << page_number << ", isWrtite = " << is_write << " }" << endl; 
            printf("Request : { Page No : %d, %s}\n", page_number, is_write ? "write" : "read");
            // Check if the page is in the page table
            if (!pageTable[page_number].is_valid) {
                page_miss_count++;
                // Page fault: allocate a frame for the page
                int frame_number = allocateFrame();
                if (frame_number == -1) {
                    // No free frames available, need to evict a page
                    removePageFromMainMemory();
                    frame_number = allocateFrame();
                }

                // Load the page into the frame
                loadPageFromSwap(page_number, frame_number, memory);

                // Update the page table entry
                pageTable[page_number].frame_number = frame_number;
                pageTable[page_number].is_valid = true;
                pageTable[page_number].is_dirty = false;
            }
            else {
                page_hit_count++;
            }

            // Update the page table entry
            pageTable[page_number].last_access_timestamp = current_timestamp;
            pageTable[page_number].is_dirty |= is_write;

            // if(current_timestamp % 1000 == 0) 
            printPageTable();
            printFrameTable();
        }

        void printPagingSystemStats() {
            double hit = (double)page_hit_count / (page_hit_count + page_miss_count);
            double miss = 1 - hit;

            printf("\nStatistics :\n");
            printf(" -------------------------------------------------\n");
            printf("| No of page hits    | %5d / %5d              |\n", page_hit_count, page_hit_count + page_miss_count);
            printf("| No of page Miss    | %5d / %5d              |\n", page_miss_count, page_hit_count + page_miss_count);
            printf("| Hit rate           | %13lf              |\n", hit);
            printf("| Miss rate          | %13lf              |\n", miss);
            printf("| EMAT Demand Paging | %13lf microseconds |\n", hit*10 + miss*1000); 
            printf("| EMAT Non Paging    | %13lf microseconds |\n", 10.0); 
            printf(" -------------------------------------------------\n\n");
            
            // printf("----------------------------------------------------------------\n");



        }
};

 vector<AccessRequest> runDataGenerator() {
     // Initialize the random number generator
    // random_device rd;
    // mt19937 gen(rd());
    // uniform_real_distribution<double> dis(0.0, 1.0);

    // Initialize the page table and memory
    int num_pages = MEMORY_SIZE / PAGE_SIZE;
    // Page page_table[num_pages];
    // char memory[MEMORY_SIZE];

    // Initialize the data generator
    int current_timestamp_ = 0;
    vector<AccessRequest> access_requests(MAX_ACCESS_REQUESTS);

    // Generate a sequence of random memory access requests
    for (int i = 0; i < MAX_ACCESS_REQUESTS; i++) {
        // Generate a random page number and access type
        int page_number = rand()%num_pages;
        page_number = rand()%MAX_NUM_PAGES;
        bool is_write = ((double)(rand()%100)/100.0 < WRITE_PROBABILITY);
        cout << "iswt : " << is_write << endl;
        // Check if the access request is a read or write
        if (is_write) {
            // Write access
            access_requests[i] = { page_number, true };
        } else {
            // Read access
            access_requests[i] = { page_number, false };
        }

        // Update the timestamp
        current_timestamp_++;

        // Check if the access request is part of a locality of reference
        if ( (double)(rand()%100)/100.0 < LOCALITY_OF_REFERENCE) {
            // Generate additional access requests for the same page
            for (int j = 0; j < 4; j++) {
                // Generate a random offset within the page
                int offset = rand()%PAGE_SIZE;

                // Generate a random read or write access
                if (is_write) {
                    // Write access
                    memory[page_number * PAGE_SIZE + offset] = rand() % 256;
                } else {
                    // Read access
                    char data = memory[page_number * PAGE_SIZE + offset];
                }

                // Update the timestamp
                current_timestamp_++;
            }
        }
    }
    return access_requests;
    
    
}

int main() {
 
    vector<AccessRequest> requests = runDataGenerator();

    // 0     :- FIFO 
    // other :- LRU
    int algo = 0;
    DemandPagingSystem paging_system(algo);

    // Process the memory access requests using the demand paging system
    for (int i = 0; i < MAX_ACCESS_REQUESTS; i++) {
        int page_number = requests[i].page_number;
        bool is_write = requests[i].is_write;

        // handleRequest(page_number, page_table, num_pages, memory, PAGE_SIZE, is_write, current_timestamp);
        paging_system.handleRequest(page_number, memory, is_write, current_timestamp_);
        // Update the timestamp
        current_timestamp_++;
    }

    paging_system.printPagingSystemStats();

    return 0;
}

