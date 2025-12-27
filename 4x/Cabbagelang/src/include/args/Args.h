#ifndef ARGS_ARGS_H
#define ARGS_ARGS_H

#include <argparse/argparse.hpp>
#include <CompilerInfo.h>

namespace args {

extern argparse::ArgumentParser cabbage;
void init();

}

#endif // ARGS_ARGS_H