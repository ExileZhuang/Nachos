// exception.cc 
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.  
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "syscall.h"

//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2. 
//
// And don't forget to increment the pc before returning. (Or else you'll
// loop making the same system call forever!
//
//	"which" is the kind of exception.  The list of possible exceptions 
//	are in machine.h.
//----------------------------------------------------------------------
void AdvancePC(){
    machine->WriteRegister(PrevPCReg,machine->ReadRegister(PCReg));		// 前一个PC
    machine->WriteRegister(PCReg,machine->ReadRegister(NextPCReg));		// 当前PC
    machine->WriteRegister(NextPCReg,machine->ReadRegister(NextPCReg)+4);	// 下一个PC
}

void StartProcess(int spaceId) {
	// printf("spaceId:%d\n",spaceId);
    ASSERT(currentThread->userProgramId() == spaceId);
    currentThread->space->InitRegisters();     // 设置寄存器初值
    currentThread->space->RestoreState();      // 加载页表寄存器
    machine->Run();             // 运行
    ASSERT(FALSE);
}


void
ExceptionHandler(ExceptionType which)
{
    //printf("This is Lab7/8 Exception\n");
    int type = machine->ReadRegister(2);

    if (which == SyscallException) {
        switch(type){
            case SC_Halt:{
                DEBUG('a',"Shutdown,initiated by user program");
                interrupt->Halt();
                break;
            }
            case SC_Exec:{
                printf("This is SC_Exec, CurrentThreadId: %d\n",(currentThread->space)->getSpaceId());
                int addr = machine->ReadRegister(4);
                char filename[50];
                for(int i = 0; ; i++){
                    machine->ReadMem(addr+i,1,(int *)&filename[i]);
                    if(filename[i] == '\0') break;
                }
                OpenFile *executable = fileSystem->Open(filename);
                if(executable == NULL) {
                    printf("Unable to open file %s\n",filename);
                    return;
                }
                printf("%s\n",filename);// 建立新地址空间
                AddrSpace *space = new AddrSpace(executable);  // 输出新分配的地址空间
                delete executable;	// 关闭文件
                // 建立新线程
                Thread *thread = new Thread(filename);
                printf("new Thread, SpaceId: %d, Name: %s\n",space->getSpaceId(),filename);
                // 将该用户进程映射到核心线程上
                thread->space = space;
                thread->Fork(StartProcess,(int)space->getSpaceId());
                machine->WriteRegister(2,space->getSpaceId());
                AdvancePC();
                break;
            }
            case SC_Exit:{
                printf("This is SC_Exit, CurrentThreadId: %d\n",(currentThread->space)->getSpaceId());
                int exitCode = machine->ReadRegister(4);
                machine->WriteRegister(2,exitCode);
                currentThread->setExitCode(exitCode);
                if(exitCode == 99)
                    scheduler->emptyList(scheduler->getTerminatedList());
                delete currentThread->space;
                currentThread->Finish();
                AdvancePC();
                break;
            }
            case SC_Join:{
                printf("This is SC_Join, CurrentThreadId: %d\n",(currentThread->space)->getSpaceId());
                int SpaceId = machine->ReadRegister(4);
                currentThread->Join(SpaceId);
                // waitProcessExitCode —— 返回 Joinee 的退出码
                machine->WriteRegister(2, currentThread->waitExitCode());
                AdvancePC();
                break;
            }
            case SC_Yield:{
                printf("This is SC_Yield, CurrentThreadId: %d\n",(currentThread->space)->getSpaceId());
                currentThread->Yield();
                AdvancePC();
                break;
            }
            default:{
                printf("Unexpted enception\n");
                ASSERT(FALSE);
            }
        }
    } else {
	printf("Unexpected user mode exception %d %d\n", which, type);
	ASSERT(FALSE);
    }
}
