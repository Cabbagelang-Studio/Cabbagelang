#include <args/Args.h>

namespace args {

argparse::ArgumentParser cabbage(PROGRAM_NAME, VERSION);

void init() {
    cabbage.add_argument("-p", "--parse-only")
        .help("Only parse the file, getting the AST\nUseful for debugging or IDE grammar highlighting")
        .flag();
    ;
}

}