#include"syscall.h"
int main(){
    SpaceId pid=Exec("../test/exit.noff");
    Join(pid);
    Exit(0);
}