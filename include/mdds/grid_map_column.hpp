/*************************************************************************
 *
 * Copyright (c) 2011-2012 Kohei Yoshida
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 ************************************************************************/

#ifndef __MDDS_GRID_MAP_COLUMN_HPP__
#define __MDDS_GRID_MAP_COLUMN_HPP__

#include "mdds/default_deleter.hpp"
#include "mdds/compat/unique_ptr.hpp"

#include <vector>
#include <algorithm>
#include <cassert>

#include <boost/noncopyable.hpp>

#define USE_CELL_BLOCK 1

namespace mdds { namespace __gridmap {

/**
 * Each column consists of a series of blocks, and each block stores a
 * series of non-empty cells of identical type.
 */
template<typename _Trait>
class column
{
public:
    typedef typename _Trait::cell_type cell_type;
    typedef typename _Trait::cell_block_type cell_block_type;
    typedef typename _Trait::cell_category_type cell_category_type;
    typedef typename _Trait::row_key_type row_key_type;

private:
    typedef typename _Trait::cell_delete_handler cell_delete_handler;
    typedef typename _Trait::cell_block_delete_handler cell_block_delete_handler;

#if USE_CELL_BLOCK
    struct block : boost::noncopyable
    {
        row_key_type m_size;
        cell_block_type* mp_data;

        block();
        block(row_key_type _size);
        ~block();
    };
#else
    /**
     * Data for non-empty block.  Cells are stored here.
     */
    struct block_data : boost::noncopyable
    {
        cell_category_type m_type;
        std::vector<cell_type*> m_cells;

        block_data(cell_category_type _type, size_t _init_size = 0);
        ~block_data();
    };

    /**
     * Empty or non-empty block.
     */
    struct block : boost::noncopyable
    {
        row_key_type m_size;
        block_data* mp_data;
        bool m_empty;

        block();
        block(row_key_type _size);
        ~block();
    };
#endif
    column(); // disabled
public:
    column(row_key_type max_row);
    ~column();

    void set_cell(row_key_type row, cell_category_type cat, cell_type* cell);
    const cell_type* get_cell(row_key_type row) const;

private:
    std::vector<block*> m_blocks;
    row_key_type m_max_row;
};

#if USE_CELL_BLOCK
template<typename _Trait>
column<_Trait>::block::block() : m_size(0), mp_data(NULL) {}

template<typename _Trait>
column<_Trait>::block::block(row_key_type _size) : m_size(_size), mp_data(NULL) {}

template<typename _Trait>
column<_Trait>::block::~block()
{
    static cell_block_delete_handler hdl;
    hdl(mp_data);
}
#else
template<typename _Trait>
column<_Trait>::block_data::block_data(cell_category_type _type, size_t _init_size) :
    m_type(_type), m_cells(_init_size, NULL) {}

template<typename _Trait>
column<_Trait>::block_data::~block_data()
{
    std::for_each(m_cells.begin(), m_cells.end(), cell_delete_handler());
}

#endif

template<typename _Trait>
column<_Trait>::column(row_key_type max_row) : m_max_row(max_row)
{
    // Initialize with an empty block that spans from 0 to max.
    m_blocks.push_back(new block(max_row));
}

template<typename _Trait>
column<_Trait>::~column()
{
    std::for_each(m_blocks.begin(), m_blocks.end(), default_deleter<block>());
}

template<typename _Trait>
void column<_Trait>::set_cell(row_key_type row, cell_category_type cat, cell_type* cell)
{
#if USE_CELL_BLOCK
#else
    if (!cell)
        return;

    unique_ptr<cell_type, cell_delete_handler> p(cell);

    // Find the right block ID from the row ID.
    row_key_type start_row = 0; // row ID of the first cell in a block.
    row_key_type block_index = 0;
    for (size_t i = 0, n = m_blocks.size(); i < n; ++i)
    {
        const block& blk = *m_blocks[i];
        if (row < start_row + blk.m_size)
        {
            // Row is in this block.
            block_index = i;
            break;
        }

        // Specified row is not in this block.
        start_row += blk.m_size;
    }

    block& blk = *m_blocks[block_index];
    assert(blk.m_size > 0); // block size should never be zero at any time.

    if (blk.m_empty)
    {
        // This is an empty block.
    }
    else if (blk.mp_data->m_type == cat)
    {
        // This block is of the same type as the cell being inserted.
        assert(blk.mp_data);
        block_data& data = *blk.mp_data;
        row_key_type i = row - start_row;
        assert(data.m_cells.size() > static_cast<size_t>(i));
        delete data.m_cells[i];
        data.m_cells[i] = p.release();
    }
    else if (row == start_row)
    {
        // Insertion point is at the start of the block.
    }
    else if (row == (start_row + blk.m_size - 1))
    {
        // Insertion point is at the end of the block.
    }
    else
    {
        // Insertion point is somewhere in the middle of the block.
    }
#endif
}

template<typename _Trait>
const typename column<_Trait>::cell_type*
column<_Trait>::get_cell(row_key_type row) const
{
#if USE_CELL_BLOCK
    return NULL;
#else
    row_key_type start_row = 0;
    for (size_t i = 0, n = m_blocks.size(); i < n; ++i)
    {
        const block& blk = *m_blocks[i];
        if (row >= start_row + blk.m_size)
        {
            // Specified row is not in this block.
            start_row += blk.m_size;
            continue;
        }

        if (blk.m_empty)
            // empty cell block.
            return NULL;

        assert(row >= start_row);
        assert(blk.mp_data); // data for non-empty blocks should never be NULL.
        assert(blk.m_size == static_cast<row_key_type>(blk.mp_data->m_cells.size()));
        row_key_type idx = row - start_row;
        return blk.mp_data->m_cells[idx];
    }
    return NULL;
#endif
}

}}

#endif
