// openfile.cc 
//	Routines to manage an open Nachos file.  As in UNIX, a
//	file must be open before we can read or write to it.
//	Once we're all done, we can close it (in Nachos, by deleting
//	the OpenFile data structure).
//
//	Also as in UNIX, for convenience, we keep the file header in
//	memory while the file is open.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "filehdr.h"
#include "openfile.h"
#include "system.h"

//----------------------------------------------------------------------
// OpenFile::OpenFile
// 	Open a Nachos file for reading and writing.  Bring the file header
//	into memory while the file is open.
//
//	"sector" -- the location on disk of the file header for this file
//----------------------------------------------------------------------

OpenFile::OpenFile(int sector)
{ 
    hdr = new FileHeader;
    hdr->FetchFrom(sector); //获取文件头
    seekPosition = 0;
    hdrSector=sector;
}

//----------------------------------------------------------------------
// OpenFile::~OpenFile
// 	Close a Nachos file, de-allocating any in-memory data structures.
//----------------------------------------------------------------------

OpenFile::~OpenFile()
{
    delete hdr;
}

//----------------------------------------------------------------------
// OpenFile::Seek
// 	Change the current location within the open file -- the point at
//	which the next Read or Write will start from.
//
//	"position" -- the location within the file for the next Read/Write
//----------------------------------------------------------------------

void
OpenFile::Seek(int position)
{
    seekPosition = position;
}	

//----------------------------------------------------------------------
// OpenFile::Read/Write
// 	Read/write a portion of a file, starting from seekPosition.
//	Return the number of bytes actually written or read, and as a
//	side effect, increment the current position within the file.
//
//	Implemented using the more primitive ReadAt/WriteAt.
//
//	"into" -- the buffer to contain the data to be read from disk 
//	"from" -- the buffer containing the data to be written to disk 
//	"numBytes" -- the number of bytes to transfer
//----------------------------------------------------------------------

int
OpenFile::Read(char *into, int numBytes)
{
   int result;
#ifdef FILESYS
   result = ReadAt(into,numBytes,0);
#else  
   result = ReadAt(into, numBytes, seekPosition);
   seekPosition += result;
#endif
   return result;
}

int
OpenFile::Write(char *into, int numBytes)
{
   int result = WriteAt(into, numBytes, seekPosition);
   seekPosition += result;
   return result;
}

//----------------------------------------------------------------------
// OpenFile::ReadAt/WriteAt
// 	Read/write a portion of a file, starting at "position".
//	Return the number of bytes actually written or read, but has
//	no side effects (except that Write modifies the file, of course).
//
//	There is no guarantee the request starts or ends on an even disk sector
//	boundary; however the disk only knows how to read/write a whole disk
//	sector at a time.  Thus:
//
//	For ReadAt:
//	   We read in all of the full or partial sectors that are part of the
//	   request, but we only copy the part we are interested in.
//	For WriteAt:
//	   We must first read in any sectors that will be partially written,
//	   so that we don't overwrite the unmodified portion.  We then copy
//	   in the data that will be modified, and write back all the full
//	   or partial sectors that are part of the request.
//
//	"into" -- the buffer to contain the data to be read from disk 
//	"from" -- the buffer containing the data to be written to disk 
//	"numBytes" -- the number of bytes to transfer
//	"position" -- the offset within the file of the first byte to be
//			read/written
//----------------------------------------------------------------------

int
OpenFile::ReadAt(char *into, int numBytes, int position)
{
    int fileLength = hdr->FileLength();
    int i, firstSector, lastSector, numSectors;
    char *buf;

    if ((numBytes <= 0) || (position >= fileLength))
    	return 0; 				// check request
    if ((position + numBytes) > fileLength)		
	numBytes = fileLength - position;
    DEBUG('f', "Reading %d bytes at %d, from file of length %d.\n", 	
			numBytes, position, fileLength);

    firstSector = divRoundDown(position, SectorSize);
    lastSector = divRoundDown(position + numBytes - 1, SectorSize);
    numSectors = 1 + lastSector - firstSector;

    // read in all the full and partial sectors that we need
    buf = new char[numSectors * SectorSize];
    for (i = firstSector; i <= lastSector; i++)	
        synchDisk->ReadSector(hdr->ByteToSector(i * SectorSize), 
					&buf[(i - firstSector) * SectorSize]);

    // copy the part we want
    bcopy(&buf[position - (firstSector * SectorSize)], into, numBytes);
    delete [] buf;
    return numBytes;
}

//-----------------------------------------------------------------------

//试图从文件尾部追加另一个文件内容
 int         //数据来源文件，文件大小，文件输入位置
 OpenFile::WriteAt(char *from, int numBytes, int position)
 {
     int fileLength = hdr->FileLength();//从要写入文件的文件头读取文件长度
     int i, firstSector, lastSector, numSectors;
     bool firstAligned, lastAligned;
     char *buf;
   
     
     
     if ((numBytes <= 0) || (position > fileLength))
 	return -1;//参数错误  

     if ((position + numBytes) > fileLength)//说明文件需要扩展
        {
           int incrementBytes = (position + numBytes) - fileLength;
           BitMap *freeBitMap = fileSystem->getBitMap();
           bool hdrRet;
           hdrRet = hdr->Allocate(freeBitMap,fileLength,incrementBytes);
           if(!hdrRet)
             return -1;
           fileSystem->setBitMap(freeBitMap);
        }


     DEBUG('f', "Writing %d bytes at %d, from file of length %d.\n", 	
 			numBytes, position, fileLength);

     //确定第一个扇区
     firstSector = divRoundDown(position, SectorSize);
     //确定最后一个扇区
     lastSector = divRoundDown(position + numBytes - 1, SectorSize);
     //输入数据涵盖扇区数
     numSectors = 1 + lastSector - firstSector;
     //创建完整的数据缓冲区
     buf = new char[numSectors * SectorSize];

     //确定起始位置是否为扇区开头，结束位置是否为扇区结尾
     firstAligned = (bool)(position == (firstSector * SectorSize));
     lastAligned = (bool)((position + numBytes) == ((lastSector + 1) * SectorSize));

// read in first and last sector, if they are to be partially modified
 //如果起始位置不再扇区开头或者结尾，则将开头和结尾扇区的全部内容放入缓冲区中
     if (!firstAligned)
         ReadAt(buf, SectorSize, firstSector * SectorSize);	
     if (!lastAligned && ((firstSector != lastSector) || firstAligned))
         ReadAt(&buf[(lastSector - firstSector) * SectorSize], 
 				SectorSize, lastSector * SectorSize);	

 // copy in the bytes we want to change 
 //将from中的数据拷贝道缓冲区的对应区域中
     bcopy(from, &buf[position - (firstSector * SectorSize)], numBytes);

 // write modified sectors back    
 //将缓冲区数据写入道Openfile的对应扇区中
     for (i = firstSector; i <= lastSector; i++)	
         synchDisk->WriteSector(hdr->ByteToSector(i * SectorSize), 
 					&buf[(i - firstSector) * SectorSize]);
     delete [] buf;
     return numBytes;
 }


//----------------------------------------------------------------------
// OpenFile::Length
// 	Return the number of bytes in the file.
//----------------------------------------------------------------------

int
OpenFile::Length() 
{ 
    return hdr->FileLength(); 
}

//--------------------------------------------------
void OpenFile::WriteBack()
{
    hdr->WriteBack(hdrSector);
}

//---------------------------------------------------

int OpenFile::WriteStdout(char *from,int numBytes)
{
    int file = 1;//stdout
    WriteFile(file,from,numBytes);
    return numBytes;
}

//---------------------------------------------------

int OpenFile::ReadStdin(char *into,int numBytes)
{
    int file = 0;//stdin
    return ReadPartial(file,into,numBytes);
}

