/****************************************************************************
MIT License

Copyright (c) 2026 Guillaume Boissé

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

#ifndef GFX_INCLUDE_GFX_TYPES_H
#define GFX_INCLUDE_GFX_TYPES_H

#include "gfx_core.h"

#include <vector>  // std::vector
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

void *gfxMalloc(size_t size);

void gfxFree(void *pointer);

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

    TYPE       *at(uint32_t index);
    TYPE       &operator[](uint32_t index);
    TYPE const *at(uint32_t index) const;
    TYPE const &operator[](uint32_t index) const;
    bool        has(uint32_t index) const;
    bool        empty() const;

    TYPE &insert(uint32_t index);
    TYPE &insert(uint32_t index, TYPE const &object);
    bool  erase(uint32_t index);
    void  clear();

    TYPE       *data();
    uint32_t    size() const;
    TYPE const *data() const;
    uint32_t    capacity() const;

    uint32_t get_index(uint32_t packed_index) const;
    uint32_t get_packed_index(uint32_t index) const;

protected:
    void reserve(uint32_t capacity);

    TYPE     *data_;
    uint32_t  size_;
    uint32_t  capacity_;
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
{}

template<typename TYPE>
GfxArray<TYPE>::~GfxArray()
{
    for (uint32_t i = 0; i < size_; ++i)
    {
        data_[i].~TYPE();
    }
    gfxFree(data_);
    gfxFree(indices_);
    gfxFree(packed_indices_);
}

template<typename TYPE>
TYPE *GfxArray<TYPE>::at(uint32_t index)
{
    if (index >= capacity_)
    {
        return nullptr; // out of bounds
    }
    uint32_t const packed_index = packed_indices_[index];
    if (packed_index == 0xFFFFFFFFu)
    {
        return nullptr; // not found
    }
    return &data_[packed_index];
}

template<typename TYPE>
TYPE &GfxArray<TYPE>::operator[](uint32_t index)
{
    TYPE *object = at(index);
    GFX_ASSERT(object != nullptr);
    return *object;
}

template<typename TYPE>
TYPE const *GfxArray<TYPE>::at(uint32_t index) const
{
    if (index >= capacity_)
    {
        return nullptr; // out of bounds
    }
    uint32_t const packed_index = packed_indices_[index];
    if (packed_index == 0xFFFFFFFFu)
    {
        return nullptr; // not found
    }
    return &data_[packed_index];
}

template<typename TYPE>
TYPE const &GfxArray<TYPE>::operator[](uint32_t index) const
{
    TYPE const *object = at(index);
    GFX_ASSERT(object != nullptr);
    return *object;
}

template<typename TYPE>
bool GfxArray<TYPE>::has(uint32_t index) const
{
    if (index >= capacity_)
    {
        return false; // out of bounds
    }
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
    if (index >= capacity_)
    {
        reserve(index + 1);
    }
    uint32_t const packed_index = packed_indices_[index];
    if (packed_index != 0xFFFFFFFFu)
    {
        data_[packed_index].~TYPE();
        return *new (&data_[packed_index]) TYPE();
    }
    GFX_ASSERT(size_ < capacity_);
    indices_[size_]        = index;
    packed_indices_[index] = size_;
    return *new (&data_[size_++]) TYPE();
}

template<typename TYPE>
TYPE &GfxArray<TYPE>::insert(uint32_t index, TYPE const &object)
{
    GFX_ASSERT(index < 0xFFFFFFFFu);
    if (index >= capacity_)
    {
        reserve(index + 1);
    }
    uint32_t const packed_index = packed_indices_[index];
    if (packed_index != 0xFFFFFFFFu)
    {
        data_[packed_index].~TYPE();
        return *new (&data_[packed_index]) TYPE(object);
    }
    GFX_ASSERT(size_ < capacity_);
    indices_[size_]        = index;
    packed_indices_[index] = size_;
    return *new (&data_[size_++]) TYPE(object);
}

template<typename TYPE>
bool GfxArray<TYPE>::erase(uint32_t index)
{
    GFX_ASSERT(index < capacity_);
    uint32_t const packed_index = packed_indices_[index];
    if (packed_index == 0xFFFFFFFFu)
    {
        return false;
    }
    GFX_ASSERT(size_ > 0); // should never happen
    if (packed_index != size_ - 1)
    {
        std::swap(data_[packed_index], data_[size_ - 1]);
        indices_[packed_index]                  = indices_[size_ - 1];
        packed_indices_[indices_[packed_index]] = packed_index;
    }
    data_[--size_].~TYPE();
    packed_indices_[index] = 0xFFFFFFFFu;
    indices_[size_]        = 0;
    return true;
}

template<typename TYPE>
void GfxArray<TYPE>::clear()
{
    for (uint32_t i = 0; i < size_; ++i)
    {
        packed_indices_[indices_[i]] = 0xFFFFFFFFu;
        indices_[i]                  = 0;
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
    uint32_t const new_capacity   = GFX_MAX(capacity_ + ((capacity_ + 2) >> 1), capacity);
    TYPE          *data           = (TYPE *)gfxMalloc(new_capacity * sizeof(TYPE));
    uint32_t      *indices        = (uint32_t *)gfxMalloc(new_capacity * sizeof(uint32_t));
    uint32_t      *packed_indices = (uint32_t *)gfxMalloc(new_capacity * sizeof(uint32_t));
    for (uint32_t i = 0; i < capacity_; ++i)
    {
        if (i < size_)
        {
            new (&data[i]) TYPE(std::move(data_[i]));
            data_[i].~TYPE();
        }
        indices[i]        = indices_[i];
        packed_indices[i] = packed_indices_[i];
    }
    for (uint32_t i = capacity_; i < new_capacity; ++i)
    {
        indices[i]        = 0;
        packed_indices[i] = 0xFFFFFFFFu;
    }
    gfxFree(data_);
    gfxFree(indices_);
    gfxFree(packed_indices_);
    data_           = data;
    indices_        = indices;
    packed_indices_ = packed_indices;
    capacity_       = new_capacity;
}

//!
//! Freelist allocator.
//!

class GfxFreelist
{
    GFX_NON_COPYABLE(GfxFreelist);

public:
    GfxFreelist();
    explicit GfxFreelist(char const *name);
    ~GfxFreelist();

    bool empty() const;

    uint32_t allocate_slot();
    uint32_t allocate_slots(uint32_t slot_count);
    void     free_slot(uint32_t slot);
    uint32_t size() const;
    void     clear();

    uint32_t calculate_free_slot_count() const;

protected:
    void grow(uint32_t slot_count = 1);

    char     *name_;
    uint32_t *slots_;
    uint32_t *slot_counts_;
    uint32_t  next_slot_;
    uint32_t  size_;
};

//!
//! Handle dispenser.
//!

class GfxHandles
{
    GFX_NON_COPYABLE(GfxHandles);

public:
    GfxHandles();
    explicit GfxHandles(char const *name);
    ~GfxHandles();

    bool empty() const;

    uint64_t allocate_handle();
    bool     acquire_handle(uint64_t handle);
    uint64_t get_handle(uint32_t index) const;
    bool     has_handle(uint64_t handle) const;
    bool     free_handle(uint64_t handle);

    uint32_t calculate_free_handle_count() const;

protected:
    void grow(uint32_t handle_count = 1);

    char     *name_;
    uint64_t *handles_;
    uint32_t  next_handle_;
    uint32_t  capacity_;
};

#endif //! GFX_INCLUDE_GFX_TYPES_H
