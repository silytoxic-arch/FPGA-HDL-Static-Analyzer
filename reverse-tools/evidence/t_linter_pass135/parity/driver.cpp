#include "parser/vendor/yosys_preproc.h"
#include <iostream>
int main(int argc,char** argv) {
    using namespace fpga::parser::vendorpp;
    define_map_t cache;cache.clear();
    for(int i=1;i<argc;++i) {
        budget={};budget.current_file=argv[i];
        define_map_t configured;configured.add("SYNTHESIS","1");
        std::ifstream input(argv[i],std::ios::binary);
        if(!input) return 3;
        try { std::cout<<frontend_verilog_preproc(input,argv[i],configured,cache,{}); }
        catch(const std::exception& error) { std::cerr<<error.what();return 2; }
    }
}
