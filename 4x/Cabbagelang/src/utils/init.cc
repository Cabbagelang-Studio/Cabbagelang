#include <utils/init.h>

namespace utils {

// Initialize
void init(int argc, char** argv){
    // Check the size of the pointer
    if(sizeof(void*) == 8)
        bit_size = BIT_64;
    else
        bit_size = BIT_32;
    args::init();
    args::cabbage.parse_args(argc, argv);
}

}

