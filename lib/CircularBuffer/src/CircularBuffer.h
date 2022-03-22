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