/****************************************************************************
MIT License

Copyright (c) 2024 Guillaume Boissé

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
****************************************************************************/
#ifndef GFX_INCLUDE_GFX_CORE_H
#define GFX_INCLUDE_GFX_CORE_H

#include <cstdio>       // vprintf, etc.
#include <cstdarg>      // va_list, etc.
#include <cstdint>      // uint32_t, etc.
#include <algorithm>    // std::min(), std::max()
#include <string>       // std::string
#include <utility>      // std::swap(), std::move()
#include <vector>       // std::vector

//!
//! Constants.
//!

enum GfxResult
{
    kGfxResult_NoError = 0,
    kGfxResult_InvalidParameter,
    kGfxResult_InvalidOperation,
    kGfxResult_OutOfMemory,
    kGfxResult_InternalError,

    kGfxResult_Count
};

enum GfxConstant
{
    kGfxConstant_BackBufferCount  = 3,
    kGfxConstant_MaxRenderTarget  = 8,
    kGfxConstant_MaxAnisotropy    = 8,
    kGfxConstant_MaxNameLength    = 64,
    kGfxConstant_NumBindlessSlots = 1024
};

//!
//! Public macros.
//!

#define GFX_ASSERT(X)   // skip it

#define GFX_ASSERTMSG(X, ...)

#define GFX_MIN(X, Y)   ((std::min)(X, Y))

#define GFX_MAX(X, Y)   ((std::max)(X, Y))

#define GFX_SNPRINTF(BUFFER, SIZE, FORMAT, ...) \
    _snprintf_s(BUFFER, SIZE, _TRUNCATE, FORMAT, __VA_ARGS__)

#define GFX_NON_COPYABLE(TYPE)  \
    TYPE(TYPE const &) = delete; TYPE &operator =(TYPE const &) = delete

#define GFX_ALIGN(VAL, ALIGN)   \
    (((VAL) + (static_cast<decltype(VAL)>(ALIGN) - 1)) & ~(static_cast<decltype(VAL)>(ALIGN) - 1))

#define GFX_BREAKPOINT          \
    GFX_MULTI_LINE_MACRO_BEGIN  \
        if(IsDebuggerPresent()) \
            DebugBreak();       \
    GFX_MULTI_LINE_MACRO_END

#define GFX_TRY(X)                                                          \
    GFX_MULTI_LINE_MACRO_BEGIN                                              \
        GfxResult const _res = (GfxResult)(X);                              \
        if(_res != kGfxResult_NoError)                                      \
            return GFX_SET_ERROR(_res, "`%s' failed", GFX_STRINGIFY(X));    \
    GFX_MULTI_LINE_MACRO_END

#define GFX_PRINTLN(...)                                    \
    GFX_MULTI_LINE_MACRO_BEGIN                              \
        GFX_PRINTLN_IMPL(__FILE__, __LINE__, __VA_ARGS__);  \
    GFX_MULTI_LINE_MACRO_END

#define GFX_SET_ERROR(RESULT, ...)  \
    GFX_SET_ERROR_IMPL(RESULT, __FILE__, __LINE__, __VA_ARGS__)

#define GFX_PRINT_ERROR(RESULT, ...)                                    \
    GFX_MULTI_LINE_MACRO_BEGIN                                          \
        GFX_PRINT_ERROR_IMPL(RESULT, __FILE__, __LINE__, __VA_ARGS__);  \
    GFX_MULTI_LINE_MACRO_END

//!
//! Debug macros.
//!

#ifdef _DEBUG

#undef GFX_ASSERT
#define GFX_ASSERT(X)               \
    GFX_MULTI_LINE_MACRO_BEGIN      \
        if(!(X)) GFX_BREAKPOINT;    \
    GFX_MULTI_LINE_MACRO_END

#undef GFX_ASSERTMSG
#define GFX_ASSERTMSG(X, ...)                                   \
    GFX_MULTI_LINE_MACRO_BEGIN                                  \
        if(!(X))                                                \
        {                                                       \
            GFX_PRINTLN_IMPL(__FILE__, __LINE__, __VA_ARGS__);  \
            GFX_BREAKPOINT;                                     \
        }                                                       \
    GFX_MULTI_LINE_MACRO_END

#endif //! _DEBUG

//!
//! Internal macros.
//!

#define GFX_MULTI_LINE_MACRO_BEGIN                                              \
    __pragma(warning(push))                                                     \
    __pragma(warning(disable:4127)) /* conditional expression is constant */    \
    __pragma(warning(disable:4390)) /* empty controlled statement found   */    \
    do                                                                          \
    {

#define GFX_MULTI_LINE_MACRO_END    \
    }                               \
    while(0)                        \
    __pragma(warning(pop))

#define GFX_STRINGIFY(X)    GFX_STRINGIFY2(X)

#define GFX_STRINGIFY2(X)   #X

#define GFX_INTERNAL_HANDLE(TYPE)       friend class GfxInternal; public: inline TYPE() { memset(this, 0, sizeof(*this)); }         \
                                        inline bool operator ==(TYPE const &other) const { return handle == other.handle; }         \
                                        inline bool operator !=(TYPE const &other) const { return handle != other.handle; }         \
                                        inline uint32_t getIndex() const { return (uint32_t)(handle ? handle & 0xFFFFFFFFull        \
                                                                                                    : 0xFFFFFFFFull); }             \
                                        inline operator uint32_t() const { return getIndex(); }                                     \
                                        inline operator bool() const { return !!handle; }                                           \
                                        private: uint64_t handle

#define GFX_INTERNAL_NAMED_HANDLE(TYPE) friend class GfxInternal; public: inline TYPE() { memset(this, 0, sizeof(*this)); }         \
                                        inline bool operator ==(TYPE const &other) const { return handle == other.handle; }         \
                                        inline bool operator !=(TYPE const &other) const { return handle != other.handle; }         \
                                        inline uint32_t getIndex() const { return (uint32_t)(handle ? handle & 0xFFFFFFFFull        \
                                                                                                    : 0xFFFFFFFFull); }             \
                                        inline operator uint32_t() const { return getIndex(); }                                     \
                                        inline char const *getName() const { return name; }                                         \
                                        inline void setName(char const *n) { uint32_t i = 0;                                        \
                                                                             if(n)                                                  \
                                                                               for(; n[i] && i < kGfxConstant_MaxNameLength; ++i)   \
                                                                                 name[i] = n[i]; name[i] = '\0'; }                  \
                                        inline operator bool() const { return !!handle; }                                           \
                                        private: uint64_t handle; char name[kGfxConstant_MaxNameLength + 1]

static inline char const *gfxResultGetString(GfxResult result)
{
    switch(result)
    {
    case kGfxResult_NoError:
        return "No error";
    case kGfxResult_InvalidParameter:
        return "Invalid parameter";
    case kGfxResult_InvalidOperation:
        return "Invalid operation";
    case kGfxResult_OutOfMemory:
        return "Out of memory";
    case kGfxResult_InternalError:
        return "Internal error";
    default:
        GFX_ASSERT(0);
        break;  // unknown error
    }
    return "Unknown error";
}

static inline void GFX_PRINTLN_IMPL(char const *file_name, uint32_t line_number, char const *format, ...)
{
    va_list args;
    char message[2048];
    va_start(args, format);
    GFX_ASSERT(file_name != nullptr);
    uint32_t const hsize = 16, bsize = 1024;
    uint32_t const length = (uint32_t)strlen(file_name);
    uint32_t const offset = (length >= hsize ? (length - hsize) + 1 : 0);
    uint32_t const start = (offset ? 0 : hsize - (length + 1));
    char header[hsize], body[bsize]; memset(header, ' ', hsize);
    snprintf(header + start, hsize - start, "%s", file_name + offset);
    snprintf(body, bsize - 1, "[%s:%-4u] %s\n", header, line_number, format);
    vsnprintf(message, sizeof(message), body, args);
    OutputDebugStringA(message);
    vprintf(body, args);
    va_end(args);
}

static inline void GFX_PRINT_ERROR_IMPL(GfxResult result, char const *file_name, uint32_t line_number, char const *format, ...)
{
    va_list args;
    char message[2048];
    va_start(args, format);
    GFX_ASSERT(file_name != nullptr);
    uint32_t const hsize = 16, bsize = 1024;
    uint32_t const length = (uint32_t)strlen(file_name);
    uint32_t const offset = (length >= hsize ? (length - hsize) + 1 : 0);
    uint32_t const start = (offset ? 0 : hsize - (length + 1));
    char header[hsize], body[bsize]; memset(header, ' ', hsize);
    snprintf(header + start, hsize - start, "%s", file_name + offset);
    snprintf(body, bsize - 1, "[%s:%-4u] Error: %s (0x%x: %s)\n", header, line_number, format, (uint32_t)result, gfxResultGetString(result));
    vsnprintf(message, sizeof(message), body, args);
    OutputDebugStringA(message);
    vprintf(body, args);
    va_end(args);
}

static inline GfxResult GFX_SET_ERROR_IMPL(GfxResult result, char const *file_name, uint32_t line_number, char const *format, ...)
{
    va_list args;
    char message[2048];
    va_start(args, format);
    GFX_ASSERT(file_name != nullptr);
    uint32_t const hsize = 16, bsize = 1024;
    uint32_t const length = (uint32_t)strlen(file_name);
    uint32_t const offset = (length >= hsize ? (length - hsize) + 1 : 0);
    uint32_t const start = (offset ? 0 : hsize - (length + 1));
    char header[hsize], body[bsize]; memset(header, ' ', hsize);
    snprintf(header + start, hsize - start, "%s", file_name + offset);
    snprintf(body, bsize - 1, "[%s:%-4u] Error: %s (0x%x: %s)\n", header, line_number, format, (uint32_t)result, gfxResultGetString(result));
    vsnprintf(message, sizeof(message), body, args);
    OutputDebugStringA(message);
    vprintf(body, args);
    va_end(args);
    return result;
}

//!
//! Sparse array container.
//!

template<typename TYPE>
class GfxArray
{
    GFX_NON_COPYABLE(GfxArray);

public:
    GfxArray();
    ~GfxArray();

    TYPE *at(uint32_t index);
    TYPE &operator [](uint32_t index);
    TYPE const *at(uint32_t index) const;
    TYPE const &operator [](uint32_t index) const;
    bool has(uint32_t index) const;
    bool empty() const;

    TYPE &insert(uint32_t index);
    TYPE &insert(uint32_t index, TYPE const &object);
    bool erase(uint32_t index);
    void clear();

    TYPE *data();
    uint32_t size() const;
    TYPE const *data() const;
    uint32_t capacity() const;

    uint32_t get_index(uint32_t packed_index) const;
    uint32_t get_packed_index(uint32_t index) const;

protected:
    void reserve(uint32_t capacity);

    TYPE *data_;
    uint32_t size_;
    uint32_t capacity_;
    uint32_t *indices_;
    uint32_t *packed_indices_;
};

template<typename TYPE>
GfxArray<TYPE>::GfxArray()
    : data_(nullptr)
    , size_(0)
    , capacity_(0)
    , indices_(nullptr)
    , packed_indices_(nullptr)
{
}

template<typename TYPE>
GfxArray<TYPE>::~GfxArray()
{
    for(uint32_t i = 0; i < size_; ++i)
        data_[i].~TYPE();
    free(data_);
    free(indices_);
    free(packed_indices_);
}

template<typename TYPE>
TYPE *GfxArray<TYPE>::at(uint32_t index)
{
    if(index >= capacity_)
        return nullptr; // out of bounds
    uint32_t const packed_index = packed_indices_[index];
    if(packed_index == 0xFFFFFFFFu)
        return nullptr; // not found
    return &data_[packed_index];
}

template<typename TYPE>
TYPE &GfxArray<TYPE>::operator [](uint32_t index)
{
    TYPE *object = at(index);
    GFX_ASSERT(object != nullptr);
    return *object;
}

template<typename TYPE>
TYPE const *GfxArray<TYPE>::at(uint32_t index) const
{
    if(index >= capacity_)
        return nullptr; // out of bounds
    uint32_t const packed_index = packed_indices_[index];
    if(packed_index == 0xFFFFFFFFu)
        return nullptr; // not found
    return &data_[packed_index];
}

template<typename TYPE>
TYPE const &GfxArray<TYPE>::operator [](uint32_t index) const
{
    TYPE const *object = at(index);
    GFX_ASSERT(object != nullptr);
    return *object;
}

template<typename TYPE>
bool GfxArray<TYPE>::has(uint32_t index) const
{
    if(index >= capacity_)
        return false;   // out of bounds
    return packed_indices_[index] != 0xFFFFFFFFu;
}

template<typename TYPE>
bool GfxArray<TYPE>::empty() const
{
    return size_ == 0;
}

template<typename TYPE>
TYPE &GfxArray<TYPE>::insert(uint32_t index)
{
    GFX_ASSERT(index < 0xFFFFFFFFu);
    if(index >= capacity_) reserve(index + 1);
    uint32_t const packed_index = packed_indices_[index];
    if(packed_index != 0xFFFFFFFFu)
    {
        data_[packed_index].~TYPE();
        return *new(&data_[packed_index]) TYPE();
    }
    GFX_ASSERT(size_ < capacity_);
    indices_[size_] = index;
    packed_indices_[index] = size_;
    return *new(&data_[size_++]) TYPE();
}

template<typename TYPE>
TYPE &GfxArray<TYPE>::insert(uint32_t index, TYPE const &object)
{
    GFX_ASSERT(index < 0xFFFFFFFFu);
    if(index >= capacity_) reserve(index + 1);
    uint32_t const packed_index = packed_indices_[index];
    if(packed_index != 0xFFFFFFFFu)
    {
        data_[packed_index].~TYPE();
        return *new(&data_[packed_index]) TYPE(object);
    }
    GFX_ASSERT(size_ < capacity_);
    indices_[size_] = index;
    packed_indices_[index] = size_;
    return *new(&data_[size_++]) TYPE(object);
}

template<typename TYPE>
bool GfxArray<TYPE>::erase(uint32_t index)
{
    GFX_ASSERT(index < capacity_);
    uint32_t const packed_index = packed_indices_[index];
    if(packed_index == 0xFFFFFFFFu) return false;
    GFX_ASSERT(size_ > 0);  // should never happen
    if(packed_index != size_ - 1)
    {
        std::swap(data_[packed_index], data_[size_ - 1]);
        indices_[packed_index] = indices_[size_ - 1];
        packed_indices_[indices_[packed_index]] = packed_index;
    }
    data_[--size_].~TYPE();
    packed_indices_[index] = 0xFFFFFFFFu;
    indices_[size_] = 0;
    return true;
}

template<typename TYPE>
void GfxArray<TYPE>::clear()
{
    for(uint32_t i = 0; i < size_; ++i)
    {
        packed_indices_[indices_[i]] = 0xFFFFFFFFu;
        indices_[i] = 0;
        data_[i].~TYPE();
    }
    size_ = 0;
}

template<typename TYPE>
TYPE *GfxArray<TYPE>::data()
{
    return data_;
}

template<typename TYPE>
uint32_t GfxArray<TYPE>::size() const
{
    return size_;
}

template<typename TYPE>
TYPE const *GfxArray<TYPE>::data() const
{
    return data_;
}

template<typename TYPE>
uint32_t GfxArray<TYPE>::capacity() const
{
    return capacity_;
}

template<typename TYPE>
uint32_t GfxArray<TYPE>::get_index(uint32_t packed_index) const
{
    GFX_ASSERT(packed_index < size_);
    return indices_[packed_index];
}

template<typename TYPE>
uint32_t GfxArray<TYPE>::get_packed_index(uint32_t index) const
{
    GFX_ASSERT(index < capacity_);
    return packed_indices_[index];
}

template<typename TYPE>
void GfxArray<TYPE>::reserve(uint32_t capacity)
{
    uint32_t const new_capacity = GFX_MAX(capacity_ + ((capacity_ + 2) >> 1), capacity);
    TYPE *data = (TYPE *)malloc(new_capacity * sizeof(TYPE));
    uint32_t *indices = (uint32_t *)malloc(new_capacity * sizeof(uint32_t));
    uint32_t *packed_indices = (uint32_t *)malloc(new_capacity * sizeof(uint32_t));
    for(uint32_t i = 0; i < capacity_; ++i)
    {
        if(i < size_)
        {
            new(&data[i]) TYPE(std::move(data_[i]));
            data_[i].~TYPE();
        }
        indices[i] = indices_[i];
        packed_indices[i] = packed_indices_[i];
    }
    for(uint32_t i = capacity_; i < new_capacity; ++i)
    {
        indices[i] = 0;
        packed_indices[i] = 0xFFFFFFFFu;
    }
    free(data_);
    free(indices_);
    free(packed_indices_);
    data_ = data;
    indices_ = indices;
    packed_indices_ = packed_indices;
    capacity_ = new_capacity;
}

//!
//! Freelist allocator.
//!

class GfxFreelist
{
    GFX_NON_COPYABLE(GfxFreelist);

public:
    inline GfxFreelist();
    inline explicit GfxFreelist(char const *name);
    inline ~GfxFreelist();

    inline bool empty() const;

    inline uint32_t allocate_slot();
    inline uint32_t allocate_slots(uint32_t slot_count);
    inline void free_slot(uint32_t slot);
    inline uint32_t size() const;
    inline void clear();

    inline uint32_t calculate_free_slot_count() const;

protected:
    inline void grow(uint32_t slot_count = 1);

    char *name_;
    uint32_t *slots_;
    uint32_t *slot_counts_;
    uint32_t next_slot_;
    uint32_t size_;
};

GfxFreelist::GfxFreelist()
    : name_(nullptr)
    , slots_(nullptr)
    , slot_counts_(nullptr)
    , next_slot_(0xFFFFFFFFu)
    , size_(0)
{
}

GfxFreelist::GfxFreelist(char const *name)
    : name_(name ? (char *)malloc(strlen(name) + 1) : nullptr)
    , slots_(nullptr)
    , slot_counts_(nullptr)
    , next_slot_(0xFFFFFFFFu)
    , size_(0)
{
    if(name_) for(uint32_t i = 0; !i || name[i - 1]; ++i) name_[i] = name[i];
}

GfxFreelist::~GfxFreelist()
{
#ifdef _DEBUG
    uint32_t const free_slot_count = calculate_free_slot_count();
    if(free_slot_count < size_)
        GFX_PRINTLN("Warning: %u %s(s) not freed properly; detected memory leak", size_ - free_slot_count, name_ ? name_ : "freelist slot");
#endif //! _DEBUG
    free(name_);
    free(slots_);
    free(slot_counts_);
}

bool GfxFreelist::empty() const
{
    return calculate_free_slot_count() == size_;
}

uint32_t GfxFreelist::allocate_slot()
{
    if(next_slot_ == 0xFFFFFFFFu) grow();
    GFX_ASSERT(next_slot_ != 0xFFFFFFFFu);
    uint32_t const slot = next_slot_;
    next_slot_ = slots_[next_slot_];
    slots_[slot] = 0xFFFFFFFEu;
    slot_counts_[slot] = 1;
    return slot;
}

uint32_t GfxFreelist::allocate_slots(uint32_t slot_count)
{
    uint32_t slot = 0xFFFFFFFFu;
    if(slot_count == 0) return 0xFFFFFFFFu;
    if(slot_count == 1) return allocate_slot();
    for(uint32_t next_slot = next_slot_; next_slot != 0xFFFFFFFFu; next_slot = slots_[next_slot])
    {
        bool is_block_available = true;
        if(next_slot + slot_count > size_) continue;
        for(uint32_t i = 1; i < slot_count; ++i)
            if(slots_[next_slot + i] == 0xFFFFFFFEu)
            {
                is_block_available = false;
                break;  // block isn't available
            }
        if(is_block_available)
        {
            slot = next_slot;
            break;  // found a suitable block
        }
    }
    if(slot == 0xFFFFFFFFu)
    {
        slot = size_;
        grow(slot_count);
    }
    GFX_ASSERT(slot != 0xFFFFFFFFu);
    GFX_ASSERT(slot + slot_count <= size_);
    uint32_t previous_slot = 0xFFFFFFFFu;
    uint32_t next_slot = next_slot_; next_slot_ = 0xFFFFFFFFu;
    for(; next_slot != 0xFFFFFFFFu;)
        if(next_slot < slot || next_slot >= slot + slot_count)
        {
            if(previous_slot == 0xFFFFFFFFu) next_slot_ = next_slot;
            previous_slot = next_slot;
            next_slot = slots_[next_slot];
        }
        else
        {
            if(previous_slot != 0xFFFFFFFFu) slots_[previous_slot] = slots_[next_slot];
            uint32_t const previous_next_slot = slots_[next_slot];
            slots_[next_slot] = 0xFFFFFFFEu;
            next_slot = previous_next_slot;
        }
    for(uint32_t i = 0; i < slot_count; ++i)
        slot_counts_[i + slot] = (i == 0 ? slot_count : 0);
    return slot;
}

void GfxFreelist::free_slot(uint32_t slot)
{
    GFX_ASSERT(slot < size_); if(slot >= size_) return;
    GFX_ASSERT(slots_[slot] == 0xFFFFFFFEu);
    if(slots_[slot] != 0xFFFFFFFEu) return; // already freed
    GFX_ASSERT(slot_counts_[slot] > 0);
    if(slot_counts_[slot] == 0) return; // cannot free within a block
    uint32_t i = slot + slot_counts_[slot];
    while(i-- > slot)
    {
        GFX_ASSERT(i == slot || slot_counts_[i] == 0);
        GFX_ASSERT(slots_[i] == 0xFFFFFFFEu);
        slots_[i] = next_slot_;
        slot_counts_[i] = 0;
        next_slot_ = i;
    }
}

uint32_t GfxFreelist::size() const
{
    return size_;
}

void GfxFreelist::clear()
{
    free(slots_);
    slots_ = nullptr;
    free(slot_counts_);
    slot_counts_ = nullptr;
    next_slot_ = 0xFFFFFFFFu;
    size_ = 0;
}

uint32_t GfxFreelist::calculate_free_slot_count() const
{
    uint32_t free_slot_count = 0;
    for(uint32_t next_slot = next_slot_; next_slot != 0xFFFFFFFFu; next_slot = slots_[next_slot])
        ++free_slot_count;  // found an available slot
    return free_slot_count;
}

void GfxFreelist::grow(uint32_t slot_count)
{
    uint32_t size = size_ + slot_count;
    size += ((size + 2) >> 1);  // grow by half capacity
    uint32_t *slots = (uint32_t *)malloc(size * sizeof(uint32_t));
    uint32_t *slot_counts = (uint32_t *)malloc(size * sizeof(uint32_t));
    GFX_ASSERT(slots != nullptr && slot_counts != nullptr); // out of memory
    for(uint32_t i = 0; i < size; ++i)
        if(i < size_)
        {
            slots[i] = (slots_[i] != 0xFFFFFFFFu ? slots_[i] : size_);
            slot_counts[i] = slot_counts_[i];
        }
        else
        {
            slots[i] = (i + 1 < size ? i + 1 : 0xFFFFFFFFu);
            slot_counts[i] = 0;
        }
    next_slot_ = (next_slot_ != 0xFFFFFFFFu ? next_slot_ : size_);
    free(slot_counts_); slot_counts_ = slot_counts;
    free(slots_); slots_ = slots;
    size_ = size;
}

//!
//! Handle dispenser.
//!

class GfxHandles
{
    GFX_NON_COPYABLE(GfxHandles);

public:
    inline GfxHandles();
    inline explicit GfxHandles(char const *name);
    inline ~GfxHandles();

    inline bool empty() const;

    inline uint64_t allocate_handle();
    inline bool acquire_handle(uint64_t handle);
    inline uint64_t get_handle(uint32_t index) const;
    inline bool has_handle(uint64_t handle) const;
    inline bool free_handle(uint64_t handle);

    inline uint32_t calculate_free_handle_count() const;

protected:
    inline void grow(uint32_t handle_count = 1);

    char *name_;
    uint64_t *handles_;
    uint32_t next_handle_;
    uint32_t capacity_;
};

GfxHandles::GfxHandles()
    : name_(nullptr)
    , handles_(nullptr)
    , next_handle_(0xFFFFFFFFu)
    , capacity_(0)
{
}

GfxHandles::GfxHandles(char const *name)
    : name_(name ? (char *)malloc(strlen(name) + 1) : nullptr)
    , handles_(nullptr)
    , next_handle_(0xFFFFFFFFu)
    , capacity_(0)
{
    if(name_) for(uint32_t i = 0; !i || name[i - 1]; ++i) name_[i] = name[i];
}

GfxHandles::~GfxHandles()
{
#ifdef _DEBUG
    uint32_t const free_handle_count = calculate_free_handle_count();
    if(free_handle_count < capacity_)
        GFX_PRINTLN("Warning: %u %s(s) not freed properly; detected memory leak", capacity_ - free_handle_count, name_ ? name_ : "handle");
#endif //! _DEBUG
    free(name_);
    free(handles_);
}

bool GfxHandles::empty() const
{
    return calculate_free_handle_count() == capacity_;
}

uint64_t GfxHandles::allocate_handle()
{
    if(next_handle_ == 0xFFFFFFFFu) grow();
    uint64_t const next_handle = handles_[next_handle_];
    uint64_t const handle = ((next_handle >> 32) << 32) | static_cast<uint64_t>(next_handle_);
    next_handle_ = static_cast<uint32_t>(next_handle & 0xFFFFFFFFull);
    GFX_ASSERT(handle != 0);    // should never happen
    return handle;
}

bool GfxHandles::acquire_handle(uint64_t handle)
{
    if(!handle || !(handle >> 32)) return false;    // invalid handle
    uint32_t const target_handle = static_cast<uint32_t>(handle & 0xFFFFFFFFull);
    if(target_handle >= capacity_) grow(target_handle - capacity_ + 1); // grow our capacity
    for(uint32_t previous_handle = 0xFFFFFFFFu, next_handle = next_handle_; next_handle != 0xFFFFFFFFu;
        previous_handle = next_handle, next_handle = static_cast<uint32_t>(handles_[next_handle] & 0xFFFFFFFFull))
        if(next_handle == target_handle)
        {
            uint32_t const free_handle = static_cast<uint32_t>(handles_[next_handle] & 0xFFFFFFFFull);
            handles_[next_handle] = ((handle >> 32) << 32) | static_cast<uint64_t>(free_handle);
            if(previous_handle == 0xFFFFFFFFu)
                next_handle_ = free_handle;
            else
                handles_[previous_handle] = ((handles_[previous_handle] >> 32) << 32) | static_cast<uint64_t>(free_handle);
            return true;
        }
    return false;
}

uint64_t GfxHandles::get_handle(uint32_t index) const
{
    GFX_ASSERT(index < capacity_); if(index >= capacity_) return 0;
    uint32_t const handle_age = static_cast<uint32_t>(handles_[index] >> 32);
    return (static_cast<uint64_t>(handle_age) << 32) | static_cast<uint64_t>(index);
}

bool GfxHandles::has_handle(uint64_t handle) const
{
    if(!handle) return false;   // invalid handle
    uint32_t const next_handle = static_cast<uint32_t>(handle & 0xFFFFFFFFull);
    GFX_ASSERT(next_handle < capacity_); if(next_handle >= capacity_) return false;
    uint32_t const handle_age = static_cast<uint32_t>(handles_[next_handle] >> 32);
    return handle_age == static_cast<uint32_t>(handle >> 32);
}

bool GfxHandles::free_handle(uint64_t handle)
{
    if(!handle) return false;   // invalid handle
    uint32_t const next_handle = static_cast<uint32_t>(handle & 0xFFFFFFFFull);
    GFX_ASSERT(next_handle < capacity_); if(next_handle >= capacity_) return false;
    uint32_t handle_age = static_cast<uint32_t>(handles_[next_handle] >> 32);
    if(handle_age != static_cast<uint32_t>(handle >> 32)) return false;
    handle_age = (handle_age < 0xFFFFFFFFu ? handle_age + 1 : 1);
    handles_[next_handle] = (static_cast<uint64_t>(handle_age) << 32)
                          |  static_cast<uint64_t>(next_handle_);
    next_handle_ = next_handle; // insert back into freelist
    return true;
}

uint32_t GfxHandles::calculate_free_handle_count() const
{
    uint32_t free_handle_count = 0;
    for(uint32_t next_handle = next_handle_; next_handle != 0xFFFFFFFFu;
        next_handle = static_cast<uint32_t>(handles_[next_handle] & 0xFFFFFFFFull))
        ++free_handle_count;    // found an available handle
    return free_handle_count;
}

void GfxHandles::grow(uint32_t handle_count)
{
    uint32_t previous_handle = 0xFFFFFFFFu;
    uint32_t capacity = capacity_ + handle_count;
    capacity += ((capacity + 2) >> 1);  // grow by half capacity
    uint64_t *handles = (uint64_t *)malloc(capacity * sizeof(uint64_t));
    memcpy(handles, handles_, capacity_ * sizeof(uint64_t));
    for(uint32_t i = capacity_; i < capacity; ++i)
        handles[i] = (1ull << 32) | static_cast<uint64_t>(i + 1 < capacity ? i + 1 : 0xFFFFFFFFu);
    for(uint32_t next_handle = next_handle_; next_handle != 0xFFFFFFFFu;
        next_handle = static_cast<uint32_t>(handles[next_handle] & 0xFFFFFFFFull))
        previous_handle = next_handle;
    if(previous_handle == 0xFFFFFFFFu)
        next_handle_ = capacity_;
    else
        handles[previous_handle] = ((handles[previous_handle] >> 32) << 32) | capacity_;
    free(handles_);
    handles_ = handles;
    capacity_ = capacity;
}

#endif //! GFX_INCLUDE_GFX_CORE_H
