#include"syscall.h"
int main(){
    OpenFileId fp;
    char buffer[50];
    int sz;
    Create("Ftest");
    fp=Open("Ftest");
    Write("hello,world!",12,fp);
    sz=Read(buffer,12,fp);
    Close(fp);
    Exit(0);
}