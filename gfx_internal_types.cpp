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

#include "gfx_core.h"
#include "gfx_internal_types.h"

void *gfxMalloc(size_t size)
{
#ifdef _MSC_VER
    _Notnull_ /* Silence msvc warnings about not checking allocation failure */
#endif
    void *ret = malloc(size);
    GFX_ASSERT(ret != nullptr); // out of memory
    return ret;
}

void gfxFree(void *pointer)
{
    free(pointer);
}

//!
//! Freelist allocator.
//!

GfxFreelist::GfxFreelist()
    : name_(nullptr)
    , slots_(nullptr)
    , slot_counts_(nullptr)
    , next_slot_(0xFFFFFFFFu)
    , size_(0)
{}

GfxFreelist::GfxFreelist(char const *name)
    : name_(name ? (char *)gfxMalloc(strlen(name) + 1) : nullptr)
    , slots_(nullptr)
    , slot_counts_(nullptr)
    , next_slot_(0xFFFFFFFFu)
    , size_(0)
{
    if (name_)
    {
        for (uint32_t i = 0; !i || name[i - 1]; ++i)
        {
            name_[i] = name[i];
        }
    }
}

GfxFreelist::~GfxFreelist()
{
#ifdef _DEBUG
    uint32_t const free_slot_count = calculate_free_slot_count();
    if (free_slot_count < size_)
    {
        GFX_PRINTLN("Warning: %u %s(s) not freed properly; detected memory leak", size_ - free_slot_count,
            name_ ? name_ : "freelist slot");
    }
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
    if (next_slot_ == 0xFFFFFFFFu)
    {
        grow();
    }
    GFX_ASSERT(next_slot_ != 0xFFFFFFFFu);
    uint32_t const slot = next_slot_;
    next_slot_          = slots_[next_slot_];
    slots_[slot]        = 0xFFFFFFFEu;
    slot_counts_[slot]  = 1;
    return slot;
}

uint32_t GfxFreelist::allocate_slots(uint32_t slot_count)
{
    uint32_t slot = 0xFFFFFFFFu;
    if (slot_count == 0)
    {
        return 0xFFFFFFFFu;
    }
    if (slot_count == 1)
    {
        return allocate_slot();
    }
    for (uint32_t next_slot = next_slot_; next_slot != 0xFFFFFFFFu; next_slot = slots_[next_slot])
    {
        bool is_block_available = true;
        if (next_slot + slot_count > size_)
        {
            continue;
        }
        for (uint32_t i = 1; i < slot_count; ++i)
        {
            if (slots_[next_slot + i] == 0xFFFFFFFEu)
            {
                is_block_available = false;
                break; // block isn't available
            }
        }
        if (is_block_available)
        {
            slot = next_slot;
            break; // found a suitable block
        }
    }
    if (slot == 0xFFFFFFFFu)
    {
        slot = size_;
        grow(slot_count);
    }
    GFX_ASSERT(slot != 0xFFFFFFFFu);
    GFX_ASSERT(slot + slot_count <= size_);
    uint32_t previous_slot = 0xFFFFFFFFu;
    uint32_t next_slot     = next_slot_;
    next_slot_             = 0xFFFFFFFFu;
    for (; next_slot != 0xFFFFFFFFu;)
    {
        if (next_slot < slot || next_slot >= slot + slot_count)
        {
            if (previous_slot == 0xFFFFFFFFu)
            {
                next_slot_ = next_slot;
            }
            previous_slot = next_slot;
            next_slot     = slots_[next_slot];
        }
        else
        {
            if (previous_slot != 0xFFFFFFFFu)
            {
                slots_[previous_slot] = slots_[next_slot];
            }
            uint32_t const previous_next_slot = slots_[next_slot];
            slots_[next_slot]                 = 0xFFFFFFFEu;
            next_slot                         = previous_next_slot;
        }
    }
    for (uint32_t i = 0; i < slot_count; ++i)
    {
        slot_counts_[i + slot] = (i == 0 ? slot_count : 0);
    }
    return slot;
}

void GfxFreelist::free_slot(uint32_t slot)
{
    GFX_ASSERT(slot < size_);
    if (slot >= size_)
    {
        return;
    }
    GFX_ASSERT(slots_[slot] == 0xFFFFFFFEu);
    if (slots_[slot] != 0xFFFFFFFEu)
    {
        return; // already freed
    }
    GFX_ASSERT(slot_counts_[slot] > 0);
    if (slot_counts_[slot] == 0)
    {
        return; // cannot free within a block
    }
    uint32_t i = slot + slot_counts_[slot];
    while (i-- > slot)
    {
        GFX_ASSERT(i == slot || slot_counts_[i] == 0);
        GFX_ASSERT(slots_[i] == 0xFFFFFFFEu);
        slots_[i]       = next_slot_;
        slot_counts_[i] = 0;
        next_slot_      = i;
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
    next_slot_   = 0xFFFFFFFFu;
    size_        = 0;
}

uint32_t GfxFreelist::calculate_free_slot_count() const
{
    uint32_t free_slot_count = 0;
    for (uint32_t next_slot = next_slot_; next_slot != 0xFFFFFFFFu; next_slot = slots_[next_slot])
    {
        ++free_slot_count; // found an available slot
    }
    return free_slot_count;
}

void GfxFreelist::grow(uint32_t slot_count)
{
    uint32_t size = size_ + slot_count;
    size += ((size + 2) >> 1); // grow by half capacity
    uint32_t *slots       = (uint32_t *)gfxMalloc(size * sizeof(uint32_t));
    uint32_t *slot_counts = (uint32_t *)gfxMalloc(size * sizeof(uint32_t));
    for (uint32_t i = 0; i < size; ++i)
    {
        if (i < size_)
        {
            slots[i]       = (slots_[i] != 0xFFFFFFFFu ? slots_[i] : size_);
            slot_counts[i] = slot_counts_[i];
        }
        else
        {
            slots[i]       = (i + 1 < size ? i + 1 : 0xFFFFFFFFu);
            slot_counts[i] = 0;
        }
    }
    next_slot_ = (next_slot_ != 0xFFFFFFFFu ? next_slot_ : size_);
    free(slot_counts_);
    slot_counts_ = slot_counts;
    free(slots_);
    slots_ = slots;
    size_  = size;
}

//!
//! Handle dispenser.
//!

GfxHandles::GfxHandles()
    : name_(nullptr)
    , handles_(nullptr)
    , next_handle_(0xFFFFFFFFu)
    , capacity_(0)
{}

GfxHandles::GfxHandles(char const *name)
    : name_(name ? (char *)gfxMalloc(strlen(name) + 1) : nullptr)
    , handles_(nullptr)
    , next_handle_(0xFFFFFFFFu)
    , capacity_(0)
{
    if (name_)
    {
        for (uint32_t i = 0; !i || name[i - 1]; ++i)
        {
            name_[i] = name[i];
        }
    }
}

GfxHandles::~GfxHandles()
{
#ifdef _DEBUG
    uint32_t const free_handle_count = calculate_free_handle_count();
    if (free_handle_count < capacity_)
    {
        GFX_PRINTLN("Warning: %u %s(s) not freed properly; detected memory leak",
            capacity_ - free_handle_count, name_ ? name_ : "handle");
    }
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
    if (next_handle_ == 0xFFFFFFFFu)
    {
        grow();
    }
    uint64_t const next_handle = handles_[next_handle_];
    uint64_t const handle      = ((next_handle >> 32) << 32) | static_cast<uint64_t>(next_handle_);
    next_handle_               = static_cast<uint32_t>(next_handle & 0xFFFFFFFFull);
    GFX_ASSERT(handle != 0); // should never happen
    return handle;
}

bool GfxHandles::acquire_handle(uint64_t handle)
{
    if (!handle || !(handle >> 32))
    {
        return false; // invalid handle
    }
    uint32_t const target_handle = static_cast<uint32_t>(handle & 0xFFFFFFFFull);
    if (target_handle >= capacity_)
    {
        grow(target_handle - capacity_ + 1); // grow our capacity
    }
    for (uint32_t previous_handle = 0xFFFFFFFFu, next_handle = next_handle_; next_handle != 0xFFFFFFFFu;
        previous_handle       = next_handle,
                  next_handle = static_cast<uint32_t>(handles_[next_handle] & 0xFFFFFFFFull))
    {
        if (next_handle == target_handle)
        {
            uint32_t const free_handle = static_cast<uint32_t>(handles_[next_handle] & 0xFFFFFFFFull);
            handles_[next_handle]      = ((handle >> 32) << 32) | static_cast<uint64_t>(free_handle);
            if (previous_handle == 0xFFFFFFFFu)
            {
                next_handle_ = free_handle;
            }
            else
            {
                handles_[previous_handle] =
                    ((handles_[previous_handle] >> 32) << 32) | static_cast<uint64_t>(free_handle);
            }
            return true;
        }
    }
    return false;
}

uint64_t GfxHandles::get_handle(uint32_t index) const
{
    GFX_ASSERT(index < capacity_);
    if (index >= capacity_)
    {
        return 0;
    }
    uint32_t const handle_age = static_cast<uint32_t>(handles_[index] >> 32);
    return (static_cast<uint64_t>(handle_age) << 32) | static_cast<uint64_t>(index);
}

bool GfxHandles::has_handle(uint64_t handle) const
{
    if (!handle)
    {
        return false; // invalid handle
    }
    uint32_t const next_handle = static_cast<uint32_t>(handle & 0xFFFFFFFFull);
    GFX_ASSERT(next_handle < capacity_);
    if (next_handle >= capacity_)
    {
        return false;
    }
    uint32_t const handle_age = static_cast<uint32_t>(handles_[next_handle] >> 32);
    return handle_age == static_cast<uint32_t>(handle >> 32);
}

bool GfxHandles::free_handle(uint64_t handle)
{
    if (!handle)
    {
        return false; // invalid handle
    }
    uint32_t const next_handle = static_cast<uint32_t>(handle & 0xFFFFFFFFull);
    GFX_ASSERT(next_handle < capacity_);
    if (next_handle >= capacity_)
    {
        return false;
    }
    uint32_t handle_age = static_cast<uint32_t>(handles_[next_handle] >> 32);
    if (handle_age != static_cast<uint32_t>(handle >> 32))
    {
        return false;
    }
    handle_age            = (handle_age < 0xFFFFFFFFu ? handle_age + 1 : 1);
    handles_[next_handle] = (static_cast<uint64_t>(handle_age) << 32) | static_cast<uint64_t>(next_handle_);
    next_handle_          = next_handle; // insert back into freelist
    return true;
}

uint32_t GfxHandles::calculate_free_handle_count() const
{
    uint32_t free_handle_count = 0;
    for (uint32_t next_handle = next_handle_; next_handle != 0xFFFFFFFFu;
        next_handle           = static_cast<uint32_t>(handles_[next_handle] & 0xFFFFFFFFull))
    {
        ++free_handle_count; // found an available handle
    }
    return free_handle_count;
}

void GfxHandles::grow(uint32_t handle_count)
{
    uint32_t previous_handle = 0xFFFFFFFFu;
    uint32_t capacity        = capacity_ + handle_count;
    capacity += ((capacity + 2) >> 1); // grow by half capacity
    uint64_t *handles = (uint64_t *)gfxMalloc(capacity * sizeof(uint64_t));
    memcpy(handles, handles_, capacity_ * sizeof(uint64_t));
    for (uint32_t i = capacity_; i < capacity; ++i)
    {
        handles[i] = (1ull << 32) | static_cast<uint64_t>(i + 1 < capacity ? i + 1 : 0xFFFFFFFFu);
    }
    for (uint32_t next_handle = next_handle_; next_handle != 0xFFFFFFFFu;
        next_handle           = static_cast<uint32_t>(handles[next_handle] & 0xFFFFFFFFull))
    {
        previous_handle = next_handle;
    }
    if (previous_handle == 0xFFFFFFFFu)
    {
        next_handle_ = capacity_;
    }
    else
    {
        GFX_ASSERT(previous_handle < capacity);
        handles[previous_handle] = ((handles[previous_handle] >> 32) << 32) | capacity_;
    }
    free(handles_);
    handles_  = handles;
    capacity_ = capacity;
}
