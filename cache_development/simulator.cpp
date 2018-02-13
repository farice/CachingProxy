#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include "Cache.cpp"
#include <string>
#include <chrono>


void printCacheStats(Cache * c, std::string msg){
    std::cout<<"**************************************\n";
    std::cout << msg << std::endl;
    std::cout<<"\t# lookups: "<<c->getLookups()<<std::endl;
    std::cout<<"\t# hits: "<<c->getHits()<<std::endl;
    std::cout<<"\t# misses: "<<(c->getLookups() - c->getHits())<<std::endl;
}


int main(int argc, char ** argv)
{
    
    if((argc != 3) && (argc != 4)){
        std::cout<<"Expected format: ./simulator <config_file> <trace_file>\n";
        return 1;
    }
    
    Cache * l1i = NULL;
    Cache * l1d = NULL;
    Cache * l2i = NULL;
    Cache * l2d = NULL;
    
    size_t DRAM_ACC_TIME;
    size_t L1D_ACC_TIME;
    size_t L1I_ACC_TIME;
    size_t L2D_ACC_TIME;
    size_t L2I_ACC_TIME;
    
    bool have_l1 = false;
    bool l1_unified = false;
    bool have_l2 = false;
    bool l2_unified = false;
    
    
    std::string line;
    
    std::ifstream f(argv[1]);
    
    std::getline (f, line);
    size_t found = line.find(" ", 0);
    
    if(found != std::string::npos)
    {
        DRAM_ACC_TIME = std::stoi(line.substr(found, line.length()));
    }
    
    // Read out cache parameters from file
    // file is expected to be 26 lines
    size_t cacheParams[26] = {0};
    for(int i = 0; i < 26; i ++)
    {
        std::getline (f, line);
        size_t found = line.find(" ", 0);
        
        if(found != std::string::npos)
        {
            cacheParams[i] = std::stoi(line.substr(found, line.length()));
        }
    }
    // check if L1type was set
    if(cacheParams[0] > 0)
    {
        have_l1 = true;
        //   Cache( assoc, bsize, cap, wmalloc, rpol, tmiss )
        l1d = new Cache(cacheParams[1], cacheParams[2], cacheParams[3], cacheParams[4], cacheParams[5],
                        cacheParams[6]);
        std::cout<<"L1 Data $ configuration"<<std::endl;
        l1d->printCache();
        L1D_ACC_TIME = cacheParams[6];
        // if L1type is unified, point instruction cache to the cache we just created
        if(cacheParams[0] == 1)
        {
            l1_unified = true;
            l1i = l1d;
        }
        // if L1type is split, create separate instruction cache with parameters from file
        else
        {
            l1i = new Cache(cacheParams[7], cacheParams[8], cacheParams[9], cacheParams[10],
                            cacheParams[11], cacheParams[12]);
            std::cout<<"L1 Instruction $ configuration"<<std::endl;
            l1i->printCache();
            L1I_ACC_TIME = cacheParams[12];
        }
    }
    // check if L2type was set, if so create the data cache object for L2
    if(cacheParams[13] > 0)
    {
        have_l2 = true;
        l2d = new Cache(cacheParams[14], cacheParams[15], cacheParams[16], cacheParams[17],
                        cacheParams[18], cacheParams[19]);
        std::cout<<"L2 Data $ configuration"<<std::endl;
        l2d->printCache();
        L2D_ACC_TIME = cacheParams[19];
        // if L2type is unified, point L2 instruction cache at the L2 data cache
        if(cacheParams[13] == 1)
        {
            l2_unified = true;
            l2i = l2d;
        }
        // if L2type is split, create separate L2 instruction cache object
        else
        {
            l2i = new Cache(cacheParams[20], cacheParams[21], cacheParams[22], cacheParams[23],
                            cacheParams[24], cacheParams[25]);
            std::cout<<"L2 Instruction $ configuration"<<std::endl;
            l2i->printCache();
            L2I_ACC_TIME = cacheParams[25];
        }
    }
    
    //parse
    size_t l1_num_data_lookups = 0;
    size_t l1_num_data_misses = 0;
    size_t l1_num_instruction_lookups = 0;
    size_t l1_num_instruction_misses = 0;
    size_t l1_num_writes = 0;
    size_t l1_num_write_misses = 0;
    size_t l1_num_reads = 0;
    size_t l1_num_read_misses = 0;
    
    size_t l2_num_data_lookups = 0;
    size_t l2_num_data_misses = 0;
    size_t l2_num_instruction_lookups = 0;
    size_t l2_num_instruction_misses = 0;
    size_t l2_num_writes = 0;
    size_t l2_num_write_misses = 0;
    size_t l2_num_reads = 0;
    size_t l2_num_read_misses = 0;
    
    
    std::ifstream traceFile(argv[2]);
    
    int code;
    int addr;
    
    size_t inst_cnt = 0;
    
    std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
    
    while(traceFile >> code >> std::hex >> addr >> std::dec)
    {
        
        inst_cnt++;
        if((inst_cnt % 10000) == 0){
            std::cout<<"Inst cnt: "<<inst_cnt<<std::endl;
        }
        
        if(argc == 4){
            // wait for user input
            std::cout<<"Input 'n' to exit, other to continue: ";
            std::string wait;
            std::getline(std::cin, wait);
            if(wait == "n"){
                return 1;
            }
        }
        
        //std::cout<<"\nCode: '"<<code<<"'     Addr: '"<<std::hex<<addr<<std::dec<<"'"<<std::endl;
        
        /***************************************************************/
        // 0 = Data Read
        if(code == 0)
        {
            //std::cout<<"0 = Data Read\n";
            l1_num_data_lookups++;
            l1_num_reads++;
            
            // Generate cache request to l1 data cache
            // if ret == true, request was a HIT.  Else, MISS
            SingleLine sl;
            bool ret = l1d -> cacheRequest(code, addr, sl);
            if((sl.dirty == 1) && (l2d != NULL)){
                // assume that the block that must be written is in L2
                l2_num_writes++;
            }
            if(ret)
            {
                //std::cout<<"Data Read Hit in L1!\n";
                //totalTime += l1d -> hitTime;
            }
            // else, ret == false, MISS in L1, if we have L2 put request out to L2
            else
            {
                //std::cout<<"Data Read Miss in L1!\n";
                l1_num_data_misses++;
                l1_num_read_misses++;
                
                //totalTime += l1d -> missTime;
                if(l2d != NULL)
                {
                    l2_num_data_lookups++;
                    l2_num_reads++;
                    
                    // put a cache request for the data to L2
                    // record if request was hit or miss in L2
                    if(l2d -> cacheRequest(code, addr, sl))
                    {
                        //totalTime += l2d -> hitTime;
                        //std::cout<<"Data Read Hit in L2!\n";
                        
                    }
                    else
                    {
                        //std::cout<<"Data Read Miss in L2!\n";
                        l2_num_data_misses++;
                        l2_num_read_misses++;
                        //totalTime += l2d -> missTime;
                    }
                }
            }
        }
        /***************************************************************/
        // 1 = Data Write
        else if(code == 1){
            //std::cout<<"1 = Data Write\n";
            l1_num_data_lookups++;
            l1_num_writes++;
            
            // generate cache request to L1 data cache
            SingleLine sl;
            bool l1_request_data_write = l1d->cacheRequest(code, addr, sl);
            if((sl.dirty == 1) && (l2d != NULL)){
                l2_num_writes++;
            }
            // check if the request was a hit or miss
            
            // L1 HIT
            if(l1_request_data_write == true){
                //std::cout<<"Data Write Hit in L1!\n";
                //totalTime += l1d -> hitTime;
            }
            // L1 MISS
            else{
                l1_num_data_misses++;
                l1_num_write_misses++;
                //std::cout<<"Data Write Miss in L1!\n";
                
                // if we have a L2 cache, send request to L2, else done
                if(l2d != NULL){
                    
                    bool l2_return = false;
                    
                    // if l2d is write miss allocate
                    if(cacheParams[17] == 1){
                        // read the L2 cache, this read will bring the data in if it is not there
                        l2_return = l2d->cacheRequest(0, addr, sl);
                    }
                    // else, l2 is NOT write miss allocate, so read but dont change anything
                    else{
                        // issue a read that will not bring the data in if it is not there
                        l2_return = l2d->cacheRequest(3, addr, sl);
                    }
                    // since we just read L2, increment the stat counts and check if it was a hit or miss
                    l2_num_data_lookups++;
                    l2_num_reads++;
                    if(l2_return == false){
                        l2_num_data_misses++;
                        l2_num_read_misses++;
                    }
                    
                    if((cacheParams[17] == 0) && (l2_return == false)){
                        // do nothing. The non-destructive read returned a MISS meaning the data is not there.
                        // Since we are not allocating on write miss, we do NOT issue the write, so return.
                    }
                    else{
                        // issue the write request
                        l2_num_data_lookups++;
                        l2_num_writes++;
                        bool l2_request_data_write = l2d->cacheRequest(code, addr, sl);
                        
                        // check if the request hit in L2
                        // L2 HIT
                        if(l2_request_data_write == true){
                            //std::cout<<"Data Write Hit in L2!\n";
                            //totalTime += l2d->hitTime;
                        }
                        // L2 MISS
                        else{
                            // should NEVER be here
                            //std::cout<<"Data Write Miss in L2!\n";
                            l2_num_data_misses++;
                            l2_num_write_misses++;
                            
                        }
                    }
                }
            }
            //std::cout<<"Total time for this request: "<<time<<std::endl;
        }
        /***************************************************************/
        // 2 = Instruction Fetch
        else if(code == 2){
            //std::cout<<"2 = Instruction Fetch\n";
            l1_num_instruction_lookups++;
            l1_num_reads++;
            SingleLine sl;
            bool l1_request_instruction_fetch = l1i->cacheRequest(code, addr, sl);
            if((sl.dirty == 1) && (l2d != NULL)){
                l2_num_writes++;
            }
            // check if the request was a hit or miss
            // L1 HIT
            if(l1_request_instruction_fetch == true){
                //std::cout<<"Instruction Fetch Hit in L1!\n";
                //totalTime += l1d->hitTime;
            }
            // L1 MISS
            else{
                l1_num_instruction_misses++;
                l1_num_read_misses++;
                //std::cout<<"Instruction Fetch Miss in L1!\n";
                //totalTime += l1d->missTime;
                // if we have a L2 cache, send request to L2, else done
                if(l2i != NULL){
                    l2_num_instruction_lookups++;
                    l2_num_reads++;
                    bool l2_request_instruction_fetch = l2i->cacheRequest(code, addr, sl);
                    // check if the request hit in L2
                    // L2 HIT
                    if(l2_request_instruction_fetch == true){
                        //std::cout<<"Instruction Fetch Hit in L2!\n";
                        //totalTime += l2d->hitTime;
                    }
                    // L2 MISS
                    else{
                        //std::cout<<"Instruction Fetch Miss in L2!\n";
                        l2_num_instruction_misses++;
                        l2_num_read_misses++;
                        //totalTime += l2d->missTime;
                    }
                }
            }
        } // end Instn Fetch
        /***************************************************************/
        else{
            std::cout<<"UNKNOWN CODE, ABORT!\n";
            exit(EXIT_FAILURE);
        }
        //std::cout<<"Total time for this request: "<<time<<std::endl;
    }
    // Top of insertion -------------------------------------------------------
    
    std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();
    //std::chrono::high_resolution_clock::duration total_sim_time = end - start;
    auto total_sim_time = std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count();
    
    // L1 stats
    double average_access_time = -1.0;
    double L1_data_weight = ((double) l1d->getLookups() / (double)(l1d->getLookups() + l1i->getLookups()));
    double L1_inst_weight = ((double) l1i->getLookups() / (double)(l1d->getLookups() + l1i->getLookups()));
    double l1d_miss = ((double) l1d->getMisses() / (double)l1d->getLookups());
    double l1i_miss = ((double) l1i->getMisses() / (double)l1i->getLookups());
    
    
    if (l1i == l1d){  // data is default
        L1_data_weight = 1;
        L1_inst_weight = 0;
    }
    
    double L2_data_weight = 0;
    double L2_inst_weight = 0;
    double l2d_miss = 0;
    double l2i_miss = 0;
    
    if (have_l2){
        L2_data_weight = ((double) l2d->getLookups() / (double)(l2d->getLookups() + l2i->getLookups()));
        L2_inst_weight = ((double) l2i->getLookups() / (double)(l2d->getLookups() + l2i->getLookups()));
        l2d_miss = ((double) l2d->getMisses() / (double)l2d->getLookups());
        l2i_miss = ((double) l2i->getMisses() / (double)l2i->getLookups());
        
        if (l2i == l2d){
            L2_data_weight = 1;
            L2_inst_weight = 0;
        }
    }
    
    if(have_l1){
        //printCacheStats(l1d, "L1 data stats");
        
        if (have_l2){  // cases: <l1d,l1i,l2d,l2i>, <l1d,l1i,l2d>, <l1d,l2d,l2i>, <l1d,l2d>
            //printCacheStats(l2d, "L2 data stats");
            /*
             if (!l2_unified)
             printCacheStats(l2i, "L2 instruction stats");
             */
            if (!l1_unified){  // cases: <l1d,l1i,l2d,l2i>, <l1d,l1i,l2d>
                //printCacheStats(l1i, "L1 instruction stats");
                
                if (!l2_unified){  // case: <l1d,l1i,l2d,l2i>
                    //std::cout << "Sum of weights = " << L1_data_weight + L1_inst_weight << std::endl;
                    //std::cout << "Data = " << L1_data_weight << std::endl;
                    //std::cout << "Inst = " << L1_inst_weight << std::endl;
                    average_access_time =
                    (L1_data_weight * (L1D_ACC_TIME + (l1d_miss * (L2D_ACC_TIME + (l2d_miss * DRAM_ACC_TIME)))))
                    +
                    (L1_inst_weight * (L1I_ACC_TIME + (l1i_miss * (L2I_ACC_TIME + (l2i_miss * DRAM_ACC_TIME)))));
                }
                else{  // case <l1d,l1i,l2d>
                    std::cout << "Sum of weights = " << L1_data_weight + L1_inst_weight << std::endl;
                    std::cout << "Data = " << L1_data_weight << std::endl;
                    std::cout << "Inst = " << L1_inst_weight << std::endl;
                    
                    
                    std::cout << "L1_data_weight = " << L1_data_weight << std::endl;
                    std::cout << "L1D_ACC_TIME = " << L1D_ACC_TIME << std::endl;
                    std::cout << "l1d_miss = " << l1d_miss << std::endl;
                    std::cout << "L2D_ACC_TIME = " << L2D_ACC_TIME << std::endl;
                    std::cout << "l2d_miss = " << l2d_miss << std::endl;
                    std::cout << "DRAM_ACC_TIME = " << DRAM_ACC_TIME << std::endl;
                    std::cout << "L1_inst_weight = " << L1_inst_weight << std::endl;
                    std::cout << "L1I_ACC_TIME = " << L1I_ACC_TIME << std::endl;
                    std::cout << "l1i_miss = " << l1i_miss << std::endl;
                    std::cout << "l2d_miss = " << l2d_miss << std::endl;
                    
                    average_access_time =
                    (L1_data_weight * (L1D_ACC_TIME + (l1d_miss * (L2D_ACC_TIME + (l2d_miss * DRAM_ACC_TIME)))))
                    +
                    (L1_inst_weight * (L1I_ACC_TIME + (l1i_miss * (L2D_ACC_TIME + (l2d_miss * DRAM_ACC_TIME)))));
                }
            }
            else{  // cases: <l1d,l2d>, <l1d,l2d,l2i>
                
                if (!l2_unified){  // case: <l1d,l2d,l2i>
                    //std::cout << "Sum of L2 weights = " << L2_data_weight + L2_inst_weight << std::endl;
                    //std::cout << "Data = " << L2_data_weight << std::endl;
                    //std::cout << "Inst = " << L2_inst_weight << std::endl;
                    average_access_time =
                    L1D_ACC_TIME + (l1d_miss * ((L2_data_weight * (L2D_ACC_TIME + (l2d_miss * DRAM_ACC_TIME)))
                                                +
                                                (L2_inst_weight * (L2I_ACC_TIME + (l2i_miss * DRAM_ACC_TIME)))));
                }
                else{  // case: <l1d,l2d>
                    
                    average_access_time = L1D_ACC_TIME + (l1d_miss * (L2D_ACC_TIME + (l2d_miss * DRAM_ACC_TIME)));
                }
            }
        }
        else{  // cases: <l1d>, <l1d,l1i>
            /*
             if (!l1_unified)
             printCacheStats(l1i, "L1 instruction stats");
             */
            
            /* This works for both a unified or split L1 cache, if unified data is weighted 1, inst 0 */
            average_access_time = (L1_data_weight * (L1D_ACC_TIME + (l1d_miss * DRAM_ACC_TIME))) +
            (L1_inst_weight * (L1I_ACC_TIME + (l1i_miss * DRAM_ACC_TIME)));
        }
    }
    
    
    
    
    
    
    
    // Bottom of insertion ------------------------------------------------------
    // Output run stats
    // L1 stats
    if(have_l1){
        printCacheStats(l1d, "L1 data stats");
        delete l1d;
        // L1 split
        if(!l1_unified){
            printCacheStats(l1i, "L1 instruction stats");
            delete l1i;
        }
    }
    // L2 stats
    if(have_l2){
        printCacheStats(l2d, "L2 data stats");
        delete l2d;
        // L2 split
        if(!l2_unified){
            printCacheStats(l2i, "L2 instruction stats");
            delete l2i;
        }
    }
    
    std::cout<<"**************************************\n";
    std::cout<<"Final Run Stats:\n";
    std::cout<<"**************************************\n";
    // LEVEL 1
    if(have_l1){
        if(l1_unified){
            std::cout<<"\n-- L1u Stats\n";
            
            std::cout<<"\ttotal lookups: "<<        l1_num_data_lookups + l1_num_instruction_lookups    << std::endl;
            std::cout<<"\ttotal misses: "<<         l1_num_data_misses + l1_num_instruction_misses      << std::endl;
            
            std::cout<<"\tinstruction lookups: "<<  l1_num_instruction_lookups                          << std::endl;
            std::cout<<"\tinstruction misses: "<<   l1_num_instruction_misses                           << std::endl;
            std::cout<<"\tdata lookups: "<<         l1_num_data_lookups                                 << std::endl;
            std::cout<<"\tdata misses: "<<          l1_num_data_misses                                  << std::endl;
            std::cout<<"\treads: "<<                l1_num_reads - l1_num_instruction_lookups           << std::endl;
            std::cout<<"\tread misses: "<<          l1_num_read_misses - l1_num_instruction_misses      << std::endl;
            std::cout<<"\twrites: "<<               l1_num_writes                                       << std::endl;
            std::cout<<"\twrite misses: "<<         l1_num_write_misses                                 << std::endl;
        }
        else{
            
            std::cout<<"\n-- L1 Data\n";
            std::cout<<"\ttotal lookups: "<<        l1_num_data_lookups                             << std::endl;
            std::cout<<"\ttotal misses: "<<         l1_num_data_misses                              << std::endl;
            std::cout<<"\tinstruction lookups: "<<  0                                               << std::endl;
            std::cout<<"\tinstruction misses: "<<   0                                               << std::endl;
            std::cout<<"\tdata lookups: "<<         l1_num_data_lookups                             << std::endl;
            std::cout<<"\tdata misses: "<<          l1_num_data_misses                              << std::endl;
            std::cout<<"\treads: "<<                l1_num_reads - l1_num_instruction_lookups       << std::endl;
            std::cout<<"\tread misses: "<<          l1_num_read_misses - l1_num_instruction_misses  << std::endl;
            std::cout<<"\twrites: "<<               l1_num_writes                                   << std::endl;
            std::cout<<"\twrite misses: "<<         l1_num_write_misses                             << std::endl;
            
            std::cout<<"\n-- L1 Instruction\n";
            std::cout<<"\ttotal lookups: "<<        l1_num_instruction_lookups                      << std::endl;
            std::cout<<"\ttotal misses: "<<         l1_num_instruction_misses                       << std::endl;
            std::cout<<"\tinstruction lookups: "<<  l1_num_instruction_lookups                      << std::endl;
            std::cout<<"\tinstruction misses: "<<   l1_num_instruction_misses                       << std::endl;
            std::cout<<"\tdata lookups: "<<         0   << std::endl;
            std::cout<<"\tdata misses: "<<          0   << std::endl;
            std::cout<<"\treads: "<<                0   << std::endl;
            std::cout<<"\tread misses: "<<          0   << std::endl;
            std::cout<<"\twrites: "<<               0   << std::endl;
            std::cout<<"\twrite misses: "<<         0   << std::endl;
            
        }
    }
    if(have_l2 && l2_unified){
        std::cout<<"\n-- L2u Stats\n";
        
        std::cout<<"\ttotal lookups: "<<        l2_num_data_lookups + l2_num_instruction_lookups    << std::endl;
        std::cout<<"\ttotal misses: "<<         l2_num_data_misses + l2_num_instruction_misses      << std::endl;
        std::cout<<"\tinstruction lookups: "<<  l2_num_instruction_lookups                      << std::endl;
        std::cout<<"\tinstruction misses: "<<   l2_num_instruction_misses                       << std::endl;
        std::cout<<"\tdata lookups: "<<         l2_num_data_lookups                             << std::endl;
        std::cout<<"\tdata misses: "<<          l2_num_data_misses                              << std::endl;
        std::cout<<"\treads: "<<                l2_num_reads - l2_num_instruction_lookups       << std::endl;
        std::cout<<"\tread misses: "<<          l2_num_read_misses - l2_num_instruction_misses  << std::endl;
        std::cout<<"\twrites: "<<               l2_num_writes                                   << std::endl;
        std::cout<<"\twrite misses: "<<         l2_num_write_misses                             << std::endl;
        
    }
    if(have_l2 && !l2_unified){
        
        std::cout<<"\n-- L2 Data\n";
        std::cout<<"\ttotal lookups: "<<        l2_num_data_lookups                             << std::endl;
        std::cout<<"\ttotal misses: "<<         l2_num_data_misses                              << std::endl;
        std::cout<<"\tinstruction lookups: "<<  0                                               << std::endl;
        std::cout<<"\tinstruction misses: "<<   0                                               << std::endl;
        std::cout<<"\tdata lookups: "<<         l2_num_data_lookups                             << std::endl;
        std::cout<<"\tdata misses: "<<          l2_num_data_misses                              << std::endl;
        std::cout<<"\treads: "<<                l2_num_reads - l2_num_instruction_lookups       << std::endl;
        std::cout<<"\tread misses: "<<          l2_num_read_misses - l2_num_instruction_misses  << std::endl;
        std::cout<<"\twrites: "<<               l2_num_writes                                   << std::endl;
        std::cout<<"\twrite misses: "<<         l2_num_write_misses                             << std::endl;
        
        std::cout<<"\n-- L2 Instruction\n";
        std::cout<<"\ttotal lookups: "<<        l2_num_instruction_lookups      << std::endl;
        std::cout<<"\ttotal misses: "<<         l2_num_instruction_misses       << std::endl;
        std::cout<<"\tinstruction lookups: "<<  l2_num_instruction_lookups      << std::endl;
        std::cout<<"\tinstruction misses: "<<   l2_num_instruction_misses       << std::endl;
        std::cout<<"\tdata lookups: "<<         0 << std::endl;
        std::cout<<"\tdata misses: "<<          0 << std::endl;
        std::cout<<"\treads: "<<                0 << std::endl;
        std::cout<<"\tread misses: "<<          0 << std::endl;
        std::cout<<"\twrites: "<<               0 << std::endl;
        std::cout<<"\twrite misses: "<<         0 << std::endl;
        
    }
    std::cout << "******************************************\n";
    std::cout << "Average memory access time: " << average_access_time << " cycles" <<  std::endl;
    std::cout << "Total simulation time: " << total_sim_time << " milliseconds" <<  std::endl;
    std::cout << "Note: total simulation time will vary." << std::endl;
    std::cout << "******************************************\n";
}

