#ifndef TDF_WRITER_SIMPLE_BUFFER_HPP
#define TDF_WRITER_SIMPLE_BUFFER_HPP


template <typename T>
class SimpleBuffer
{
    T* internal_data = nullptr;
    size_t internal_size = 0;

public:
    SimpleBuffer(size_t size_) : internal_size(size_)
    {
        internal_data = (T*) malloc(size_ * sizeof(T));
        if(!internal_data) {
            throw std::bad_alloc();
        }
    }

    SimpleBuffer(const T* data_, size_t size_) : internal_size(size_)
    {
        internal_data = (T*) malloc(size_ * sizeof(T));
        if(!internal_data) {
            throw std::bad_alloc();
        }
        std::copy(data_, data_ + size_, internal_data);
    }

    ~SimpleBuffer()
    {
        if(internal_data) {
            free(internal_data);
        }
    }

    SimpleBuffer(const SimpleBuffer&) = delete;
    SimpleBuffer& operator=(const SimpleBuffer&) = delete;
    SimpleBuffer(SimpleBuffer&& other) noexcept :
        internal_data(other.internal_data),
        internal_size(other.internal_size)
    {
        other.internal_data = nullptr;
        other.internal_size = 0;
    }
    SimpleBuffer& operator=(SimpleBuffer&& other) noexcept
    {
        if(this != &other) {
            if(internal_data) {
                free(internal_data);
            }
            internal_data = other.internal_data;
            internal_size = other.internal_size;
            other.internal_data = nullptr;
            other.internal_size = 0;
        }
        return *this;
    }

    inline const T* data() const { return internal_data; }
    inline T* data() { return internal_data; }
    inline size_t size() const { return internal_size; }
};

#endif // TDF_WRITER_SIMPLE_BUFFER_HPP