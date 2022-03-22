// ======== CircularBuffer.cpp ==========
// AUTHOR       :           Felix Malmsjö
// CREATE DATE  :              2022-03-22
// PURPOSE      :         Circular buffer
// COURSE       :                  BMEF01
// ======================================
#include "CircularBuffer.h"

// Implement a circular buffer for the temperature sampling.
// By using a circular buffer we can be sampling data indefinently without
// having to worry about buffer length.
CircularBuffer::CircularBuffer(int size)
{
    len = size;
    ptr = 0;
    data = new double[len];
    data = {0};
}

void CircularBuffer::put(double value)
{
    if (data == nullptr)
    {
        data = new double[len];
        ptr = 0;
    }
    data[ptr] = value;
    ptr = (ptr+1) % len;
}

void CircularBuffer::getBuf(double *tmp)
{
    for (int i = 0; i < len; i++)
    {
        tmp[i] = data[(i + ptr) % len];
    }
}
    
CircularBuffer::~CircularBuffer()
{
    if(data != nullptr){
        delete data;
    }
}
