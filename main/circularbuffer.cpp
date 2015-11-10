// Copyright (c) 2001-2013, SCALABLE Network Technologies, Inc.  All Rights Reserved.
//                          600 Corporate Pointe
//                          Suite 1200
//                          Culver City, CA 90230
//                          info@scalable-networks.com
//
// This source code is licensed, not sold, and is subject to a written
// license agreement.  Among other things, no portion of this source
// code may be copied, transmitted, disclosed, displayed, distributed,
// translated, used as the basis for a derivative work, or used, in
// whole or in part, for any program or purpose other than its intended
// use in compliance with the license agreement as part of the QualNet
// software.  This source code and certain of the algorithms contained
// within it are confidential trade secrets of Scalable Network
// Technologies, Inc. and may not be used as the basis for any other
// software, hardware, product or service.

// CirBuffer Implementation with fixed buffer size
// /**
// PACKAGE :: CircularBuffer
// DESCRIPTION :: Implementation of CircularBuffer
// **/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "api.h"
#include "circularbuffer.h"


// /**
// API        :: getCirBufSize 
// PURPOSE    :: get the circular buffer's allocated size 
// PARAMETERS :: None
// RETURN     :: circular buffer's allocated size 
// **/
inline Int32 CircularBuffer::getCirBufSize()
{
    return m_queSz;
}

// /**
// API        :: getIndex
// PURPOSE    :: get the circular buffer's allocated size 
// PARAMETERS ::  
// + operation : Int32: Read or Write Operation
// RETURN     :: Int32: Current operation Position
// **/
inline Int32 CircularBuffer::getIndex(Int32 operation)
{
    if (operation == idRead)
    {
        return m_read;
    }
    else
    {
        return m_write;
    }
}

// /**
// API        :: lengthToEnd
// PURPOSE    :: get the circular buffer's allocated size 
// PARAMETERS ::  
// + operation : Int32: Read or Write Operation
// RETURN     :: Int32: Total length of data to be read
// **/
inline Int32 CircularBuffer::lengthToEnd(Int32 operation)
{
    if (operation == idRead)
    {
        if (m_write >= m_read)
        {
            return m_write - m_read;
        }
        else
        {
            return m_queSz - m_read;
        }
    }
    else
    {
        return m_queSz - m_write;
    }
}

// /**
// API        :: CircularBuffer
// PURPOSE    :: Constructor
// PARAMETERS ::  
// RETURN     :: None
// **/
CircularBuffer::CircularBuffer () :
    m_read(0),
    m_write(0),
    m_queSz(0),
    m_wrapType(idNoWrap),
    m_que(NULL)
{
    // default circular Buffer size taken to be CIR_BUF_SIZE
    m_que = (unsigned char*)MEM_malloc (CIR_BUF_SIZE);
}

// /**
// API        :: CircularBuffer
// PURPOSE    :: Constructor
// PARAMETERS :: 
// + queueSize : Int32: Size of the Queue
// RETURN     :: None
// **/
CircularBuffer::CircularBuffer (Int32 queueSize)
{
    m_index = 0;
    m_que = NULL;
    m_queSz = 0;
    m_read = 0;
    m_write = 0;
    m_wrapType = idNoWrap;

    // if invalid que size
    if (queueSize <= 0)
    {
        ERROR_Assert(false, "Invalid Buffer Size\n");
    }

    // create que
    m_queSz = queueSize;
    m_que = (unsigned char*)MEM_malloc (m_queSz);

    // if failed stop
    ERROR_Assert(m_que, "Invalid Buffer Size\n");
}

// /**
// API        :: CircularBuffer
// PURPOSE    :: Constructor
// PARAMETERS :: 
// + index    :  unsigned short: Circular Buffer Index 
// RETURN     :: None
// **/
CircularBuffer::CircularBuffer(unsigned short index)
{
    m_index = index;
    m_que = NULL;
    m_queSz = 0;
    m_read = 0;
    m_write = 0;
    m_wrapType = idNoWrap;

   // default circular Buffer size taken to be CIR_BUF_SIZE
    m_que = (unsigned char*)MEM_malloc (CIR_BUF_SIZE);

    // if failed stop
    ERROR_Assert(m_que, "Invalid Buffer Size\n");

}

// /**
// API        :: CircularBuffer
// PURPOSE    :: Constructor
// PARAMETERS :: 
// + index     :  unsigned short: Circular Buffer Index
// + queueSize :  Int32: Size of the queue
// RETURN     :: None
// **/
CircularBuffer::CircularBuffer(unsigned short index, Int32 queueSize)
{
    m_index = index;
    m_que = NULL;
    m_queSz = 0;
    m_read = 0;
    m_write = 0;
    m_wrapType = idNoWrap;

    // if invalid queue size
    if (queueSize <= 0)
    {
        ERROR_Assert(false, "Invalid Buffer Size\n");
    }

    // create que
    m_queSz = queueSize;
    m_que = (unsigned char*)MEM_malloc (m_queSz);

    // if failed stop
    ERROR_Assert(m_que, "Invalid Buffer Size\n");
}

// /**
// API        :: CircularBuffer
// PURPOSE    :: Destructor
// PARAMETERS ::  
// RETURN     :: none
// **/
CircularBuffer::~CircularBuffer ()
{
    release();
}

// /**
// API        :: release
// PURPOSE    :: To free the allocated memory
// PARAMETERS ::  
// RETURN     :: void : NULL
// **/
void CircularBuffer::release ()
{
    if (!m_que)
    {
        return;
    }

    // reset pos
    reset();

    // release queue
    MEM_free(m_que);
    m_que   = 0;
    m_queSz = 0;
}

// /**
// API        :: incPos
// PURPOSE    :: increment read/write position based on operation
// PARAMETERS ::  
// + increment : Int32: How much will be incremented
// + operation : Int32: Type of Operation (Read or Write )
// RETURN     :: bool: Successful or not
// **/
bool CircularBuffer::incPos (Int32 increment, Int32 operation)
{
    // if increment > queue size stop
    if (increment < 0 || increment > m_queSz)
    {
        return false;
    }

    // get position to modify
    Int32* pos = &m_write;
    if (operation == idRead)
    {
        pos = &m_read;
    }

    // get length to end of queue
    Int32 lenToEnd = lengthToEnd(operation);

    // if increment not beyond que end
    if (increment <= lenToEnd)
    {
        *pos += increment;
    }
    else
    {
        // set position to length from start
        // because of wrap
        *pos = increment - lenToEnd;
    }

    // if either position has wrapped to other
    if (m_read == m_write)
    {
        m_wrapType = idWriteWrap;
        if (operation == idRead)
        {
            m_wrapType = idReadWrap;
        }
    }
    else
    {
        m_wrapType = idNoWrap;
    }

    return true;
}

// /**
// API        :: getCount
// PURPOSE    :: gets the number of bytes to read
// PARAMETERS ::  
// + count : Int32&: the parameter to be filled up
// + operation : Int32: Type of Operation (Read or Write)
// RETURN     :: bool: successful or not
// **/
bool CircularBuffer::getCount (Int32 &count, Int32 operation)
{
    // get write and read positions
    Int32 write = m_write;
    Int32 read  = m_read;

    // get the write count
    count = 0;
    if (write == read)
    {
        count = m_queSz;
        if (m_wrapType == idWriteWrap)
        {
            count = 0;
        }
    }
    else if (write > read)
    {
        // subtract queue size from next read position then add
        // to end position
        count = (m_queSz - (write - read));
    }
    else
    {
        // subtract current end from next position to write
        count = read - write;
    }

    // if need read count then subtract read
    // count from queue size
    if (operation == idRead)
    {
        if (m_wrapType == idReadWrap)
        {
            count = 0;
        }
        else
        {
            count = m_queSz - count;
        }
    }

    // if nothing to write show none
    if (count == 0)
    {
        return false;
    }
    else
    {
        return true;
    }
}


// /**
// API        :: readFromBuffer
// PURPOSE    :: Reading the required no. of bytes from the circular buffer 
// PARAMETERS ::  
// + data   : unsigned char*: Container to which data will be read
// + length : Int32: length of data to be read
// + noIncrement:bool: Whether the read pointer is to be incremented or not
// RETURN     :: bool: successful or not
// **/
bool CircularBuffer::readFromBuffer (unsigned char* data,
    Int32 length, bool noIncrement)
{
    // check if data block too long
    if (length < 0 || length > m_queSz || length > getContentsSize())
    {
        return false;
    }

    // read length to the circular Buffer end
    Int32 lenToEnd = lengthToEnd(idRead);

    if (length <= lenToEnd)
    {
        memcpy(data, &m_que[m_read], length);
    }
    else if (lenToEnd > 0)
    {
        memcpy(data, &m_que[m_read], lenToEnd);
    }

    // copy remainder to buffer
    Int32 lenRemaining = length - lenToEnd;
    if (lenRemaining > 0)
    {
        memcpy(&data[lenToEnd], &m_que[0], lenRemaining);
    }

    // increment read position
    if (noIncrement == true)
    {
        incPos(length, idRead);
    }
    return true;
}


// /**
// API        :: write
// PURPOSE    :: Write to the circular buffer
// PARAMETERS ::  
// + data   : unsigned char*: Container to which data will be written
// + length : Int32: Length of data to be written 
// RETURN     :: bool: successful or not 
// **/
bool CircularBuffer::write (unsigned char* data, Int32 length)
{
    // show data block to long
    if (length < 0 || length > m_queSz - getContentsSize())
    {
        return false;
    }

    // read length to que end
    Int32 lenToEnd = lengthToEnd(idWrite);

    // copy to end of buffer
    if (length <= lenToEnd)
    {
        memcpy(&m_que[m_write], data, length);
    }
    else
    {
       if (lenToEnd > 0)
       {
            memcpy(&m_que[m_write], data, lenToEnd);
       }

        // copy remainder to start of buffer
        Int32 lenRemaining = length - lenToEnd;
        if (lenRemaining > 0)
        {
            memcpy(&m_que[0], &data[lenToEnd], lenRemaining);
        }
    }

    // increment write position
    incPos(length, idWrite);

    return true;
}


// /**
// API        :: read
// PURPOSE    :: To Read data from Buffer
// PARAMETERS ::  
// + buffer : unsigned char*: Container to which data will be read
// RETURN     :: bool: Succesful or not
// **/
bool CircularBuffer::read (unsigned char* buffer)
{
    // read if any data availible
    Int32 count;
    if (!getCount(count, idRead))
    {
        return false;
    }
    // setup buffer
    if (!buffer)
    {
        return false;
    }

    memset(buffer,0,count);

    // read data
    bool success = readFromBuffer ((unsigned char *) buffer, count,false);
    return success;
}


// /**
// API        :: create
// PURPOSE    :: Memory allocation for Circular Buffer
// PARAMETERS ::  
// + queueSize : Int32: Size of queue
// RETURN     :: bool : Successful or not
// **/
bool CircularBuffer::create (Int32 queueSize)
{
    // if invalid queue size
    if (queueSize <= 0)
    {
        return false;
    }

    // create queue
    m_queSz = queueSize;
    m_que = (unsigned char*) MEM_malloc (m_queSz);

    // if failed stop
    if (m_que)
    {
        return true;
    }
    else
    {
        m_queSz = 0;
        return false;
    }
}

// /**
// API        :: reset
// PURPOSE    :: reset position and wrap values
// PARAMETERS ::  
// RETURN     :: void: Null
// **/
void CircularBuffer::reset ()
{
    // reset position and wrap value
    m_read  = 0;
    m_write = 0;
    m_wrapType = idNoWrap;
}


// /**
// API        :: readWithCount
// PURPOSE    :: Read data from Buffer and pass the length of data read
// PARAMETERS ::  
// + data : unsigned char*: Container to which data will be read
// + length : Int32&: length of data read
// RETURN     :: bool :  Successful or not
// **/
bool CircularBuffer::readWithCount (unsigned char* data, Int32 & length)
{
    // show nothing copied
    length = 0;

    // read the length of the frame
    Int32 count = 0;
    if (!getCount(count, idRead))
    {
        return false;
    }

    // read the data
    bool success = readFromBuffer(data,count);

    // update length
    if (success)
    {
        length = count;
    }

    return true;
}

// /**
// API        :: CircularBuffer.getContentsSize
// PURPOSE    :: get the size of the queue's contents.  This is the
//               maximum amount of data that may be read.
// PARAMETERS :: 
// + none : void : None
// RETURN     :: Int32 : amount of data in buffer
// **/
Int32 CircularBuffer::getContentsSize()
{
    if (m_write >= m_read)
    {
        return m_write - m_read;
    }
    else
    {
        return m_queSz - (m_read - m_write);
    }
}

// /**
// API        :: CircularBuffer.dumpToStdout
// PURPOSE    :: Output the contents of the circular buffer to stdout.
//               This function is most useful when the contents of the
//               buffer are human readable strings but it will work
//               for any type of contents.
// PARAMETERS :: 
// + none : void : None
// RETURN     :: void : None
// **/
void CircularBuffer::dumpToStdout()
{
    int i = m_read;
    while (i != m_write)
    {
        printf("%c", m_que[i]);

        if (i == m_queSz - 1)
        {
            i = 0;
        }
        else
        {
            i++;
        }
    }
}
