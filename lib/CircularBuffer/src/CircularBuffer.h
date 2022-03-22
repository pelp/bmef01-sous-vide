// ========= CircularBuffer.h ===========
// AUTHOR       :           Felix Malmsjö
// CREATE DATE  :              2022-03-22
// PURPOSE      :  Circular buffer header
// COURSE       :                  BMEF01
// ======================================
class CircularBuffer
{
    private:
    double *data;
    int ptr;

    public:
    int len;
    CircularBuffer(int size);
    void put(double value);
    void getBuf(double *tmp);
    ~CircularBuffer();
};