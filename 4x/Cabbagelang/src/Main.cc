#include <utils/init.h>
#include <compiler/Compiler.h>

compiler::Compiler mainCompiler;

class Main{
    public:
    Main(int argc, char** argv){
        utils::init(argc, argv);

        mainCompiler.compile("D:/test/hello.cbg", "test/output/hello.o", false);
    }
};

int main(int argc, char** argv){
    Main main(argc, argv);
    return 0;
}