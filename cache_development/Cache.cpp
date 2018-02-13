#include <cstdio>
#include <cstdlib>
#include <vector>
#include <cmath>
#include <iostream>
#include <algorithm>

// SINGLE LINE CLASS
//   - just a container to store some data
class SingleLine {
    //private:
public:
    size_t tag;
    bool valid;
    int payload;
    int dirty;
public:
    SingleLine(): tag(0), valid(false), payload(0), dirty(0){
        //std::cout<<"SingleLine default construct\n";
    }
    SingleLine(size_t t, size_t v, size_t p) : tag(t), valid(v), payload(p), dirty(0){
        //std::cout<<"SingleLine custom construct\n";
    }
};// end SingleLine class


// SET CLASS
class Set {
    //private:
public:
    size_t associativity;
    size_t block_size;
    int replacement_policy;
    int next_line_out;
    std::vector<SingleLine> lines; // vector of lines in this set
    std::vector<size_t> LruTracker;
public:
    Set(): associativity(0), block_size(0), replacement_policy(0), next_line_out(0), lines(std::vector<SingleLine>()),LruTracker(std::vector<size_t>()) {
        //std::cout<<"Set default construct\n";
    }
    Set(size_t assoc, size_t bsize, int rpol) : associativity(assoc), block_size(bsize), replacement_policy(rpol), next_line_out(0), lines(std::vector<SingleLine>(assoc)), LruTracker(std::vector<size_t>(assoc)) {
        //std::cout<<"Set custom construct\n";
    }
    // tagLookup
    //   function to check if the tag is contained in this Set (and it is valid)
    //   return true=found, false=notfound/valid
    bool tagLookup(size_t tag, bool replace, SingleLine & sl, bool iswrite){
        for(size_t i = 0; i<lines.size(); i++){
            // if the tag is found and valid, return true immediately
            if((lines[i].tag == tag) && (lines[i].valid == true)){
                // if here, HIT
                // if this is a write instruction and the block is in the cache, set the dirty bit
                if(iswrite == true){
                    lines[i].dirty = 1;
                }
                
                updateLRU(i);
                
                return true;
            }
        }
        //std::cout<<"\tTag: "<<tag<< " NOT found\n";
        // if here, tag was not found and/or it was not valid
        if(replace == true){
            addTagEntry(tag, sl);
        }
        return false;
    }
    
    void updateLRU(size_t most_recently_used){
        for (size_t i = 0; i < LruTracker.size(); i++){
            if (i == most_recently_used){
                LruTracker[i] = 0;
            }
            else{
                LruTracker[i]++;
            }
        }
    }
    
    
    // addTagEntry
    //   function to add a tag entry to this set
    //   if there is a non-valid entry, it will be automatically added there
    //   if all entries are valid, figure out which one to replace using this set's replacement_policy
    //   return true if you had to kick something out, false if you didnt [DO WE NEED THIS?]
    bool addTagEntry(size_t tag, SingleLine & sl){
        // search for non-valid entries
        for(size_t i = 0; i<lines.size(); i++){
            // if there is a non-valid entry, automatically add the tag and set the entry to valid
            if(lines[i].valid == false){
                //std::cout<<"\tNon-valid entry found, adding tag " << tag << std::endl;
                lines[i].tag = tag;
                lines[i].valid = true;
                lines[i].dirty = 0;
                // dont care about payload
                return false;
            }
        }
        // if here, we have to kick something out
        // handle eviction based on replacement policy
        // RANDOM == 0
        if(replacement_policy == 0){
            // replace the tag at the location of next_line_out
            //std::cout<<"\tRandom Replacement: replace "<<lines[next_line_out].tag << " with "<< tag << std::endl;
            
            // update next_line_out = random number from [0,associativity)
            next_line_out = rand() % associativity;
            
            // make note of the single line that we are kicking out
            sl.tag = lines[next_line_out].tag;
            sl.valid = lines[next_line_out].valid;
            sl.payload = lines[next_line_out].payload;
            sl.dirty = lines[next_line_out].dirty;
            
            // evict the line and replace it with the new line
            lines[next_line_out].tag = tag;
            lines[next_line_out].dirty = 0;
            lines[next_line_out].valid = true;
            
        }
        // LRU == 1
        else if(replacement_policy == 1){
            // replace the tag at the location of next_line_out
            // calculate new value of next_line_out for next iter
            next_line_out = std::max_element(LruTracker.begin(), LruTracker.end()) - LruTracker.begin();
            
            //std::cout<<"\tLRU Replacement: replace "<< lines[next_line_out].tag << " with " << tag << std::endl;
            
            // make note of the single line that we are kicking out
            sl.tag = lines[next_line_out].tag;
            sl.valid = lines[next_line_out].valid;
            sl.payload = lines[next_line_out].payload;
            sl.dirty = lines[next_line_out].dirty;
            
            // evict the line and replace it with the new line
            lines[next_line_out].tag = tag;
            lines[next_line_out].dirty = 0;
            lines[next_line_out].valid = true;
            
            updateLRU(next_line_out); // update LRU tracker with most recently used
            
        }
        // shouldnt be here
        else{
            std::cout<<"UNKNOWN REPLACEMENT POLICY\n";
            exit(EXIT_FAILURE);
        }
        return true;
    }
}; // end Set class

// CACHE CLASS
class Cache {
private:
    size_t associativity;
    size_t block_size;
    size_t capacity;
    bool WM_alloc;
    int replacement_policy;
    size_t hit_time;
    size_t hit_cnt;
    size_t total_lookups;
    std::vector<Set> sets; // vector of set objects
    /*
     size_t num_data_lookups;
     size_t num_data_misses;
     size_t num_instruction_lookups;
     size_t num_instruction_misses;
     size_t num_writes;
     size_t num_write_misses;
     size_t num_reads;
     size_t num_read_misses;
     */
public:
    
    size_t getLookups(){
        return total_lookups;
    }
    size_t getHits(){
        return hit_cnt;
    }
    // Cache constructors
    //Cache(){
    //}
    size_t getMisses(){
        return this->total_lookups - this->hit_cnt;
    }
    Cache(size_t assoc, size_t bsize, size_t cap, bool WMa, int rpol, size_t tmiss) : \
    associativity(assoc), \
    block_size(bsize), \
    capacity(cap), \
    WM_alloc(WMa), \
    replacement_policy(rpol), \
    hit_time(tmiss), \
    hit_cnt(0), total_lookups(0), \
    sets(std::vector<Set>()){
        // initialize each set properly
        for(size_t i = 0; i<((cap/bsize)/assoc); i++){
            sets.push_back(Set(assoc,bsize,rpol));
        }
        
    }
    // end constructor
    void printCache(){
        std::cout<<"************************\n";
        std::cout<<"Cache Info:\n";
        std::cout<<"************************\n";
        std::cout<<"\tassoc: "<<associativity<<std::endl;
        std::cout<<"\tblk size: "<<block_size<<std::endl;
        std::cout<<"\tcapacity: "<<capacity<<std::endl;
        std::cout<<"\tWM alloc: "<<WM_alloc<<std::endl;
        std::cout<<"\tNum Sets: "<<sets.size()<<std::endl;
        for(size_t i = 0; i<sets.size(); i++){
            std::cout<<"\tsets["<<i<<"]\n";
            for(size_t j = 0; j<sets[i].lines.size(); j++){
                std::cout<<"\t\tlines["<<j<<"] : valid="<<sets[i].lines[j].valid<< "   tag="<<sets[i].lines[j].tag<<std::endl;
            }
        }
    }
    
    // Handle requests from the simulator.
    // code: 0 = Data Read, 1 = Data Write, 2 = Instruction Read
    // addr = memory address (32-bit)
    // Return value indicates hit/miss.  true=hit, false=miss
    bool cacheRequest(int code, int addr, SingleLine & sl){
        
        // calculate the tag, set_id, and offset from this address
        int num_offset_bits = ceil(log2(block_size));
        int num_setid_bits = ceil(log2((capacity/block_size)/associativity));
        
        int setid_mask = (1 << num_setid_bits)-1;
        
        // extract the bitfields by applying masks
        int setid = (addr >> num_offset_bits) & setid_mask;
        int tag = (addr >> (num_setid_bits+num_offset_bits));
        
        // based on the set_id, index into the sets vector and perform lookup of tag in that set
        bool should_write = true;
        if(((code == 1) && (WM_alloc == false)) || ((code == 3))){
            should_write = false;
        }
        
        bool iswrite = false;
        if(code == 1){
            iswrite = true;
        }
        
        //std::cout<<"Looking up tag="<<tag<<" in set="<<setid<<std::endl;
        //SingleLine sl;
        bool set_hit = sets[setid].tagLookup(tag, should_write, sl, iswrite);
        std::string request_status = (set_hit == true) ? "HIT" : "MISS";
        
        // Increment Totals
        total_lookups++;
        if(set_hit){
            hit_cnt++; // increment hit count
        }
        
        
        return set_hit;
        
    }
    
}; // end Cache class

/*
 int main(void){
 
 int assoc = 2; // was 2
 int bsize = 16;
 int cap = 32; // was 128
 bool WM = false;
 int rpol = 1;
 size_t tmiss = 0;
 
 Cache c(assoc,bsize,cap,WM,rpol,tmiss);
 std::cout<<"Cache before adding data\n";
 c.printCache();
 
 c.cacheRequest(0, 0x0C0);
 c.cacheRequest(0, 0x310);
 c.cacheRequest(0, 0x260);
 c.cacheRequest(0, 0x170);
 //c.printCache();
 c.cacheRequest(0, 0x180);
 c.cacheRequest(0, 0x350);
 c.cacheRequest(0, 0x020);
 c.cacheRequest(0, 0x2B0);
 c.printCache();
 
 c.cacheRequest(0, 0x181); // hit
 c.cacheRequest(0, 0xABC);
 c.cacheRequest(0, 0xFC1);
 c.cacheRequest(0, 0x1A5);
 c.cacheRequest(0, 0x312); // hit
 c.printCache();
 
 return 0;
 }
 */
