/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*************************************************************************
 *
 * Copyright (c) 2022 Kohei Yoshida
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

#include "test_global.hpp" // This must be the first header to be included.

#include <mdds/multi_type_vector/types.hpp>
#include <mdds/multi_type_vector/block_funcs.hpp>

#include <vector>

namespace test1 {

constexpr mdds::mtv::element_t element_type_int8 = mdds::mtv::element_type_user_start + 2;

using int8_element_block = mdds::mtv::default_element_block<element_type_int8, std::int8_t, std::vector>;

void mtv_test_element_blocks_std_vector()
{
    stack_printer __stack_printer__(__func__);

    using this_block = int8_element_block;

    static_assert(this_block::block_type == element_type_int8);

    auto* blk = this_block::create_block(10);

    assert(mdds::mtv::get_block_type(*blk) == this_block::block_type);
    assert(this_block::size(*blk) == 10u);

    this_block::delete_block(blk);
}

} // namespace test1

int main()
{
    try
    {
        test1::mtv_test_element_blocks_std_vector();
    }
    catch (const std::exception& e)
    {
        cout << "Test failed: " << e.what() << endl;
        return EXIT_FAILURE;
    }

    cout << "Test finished successfully!" << endl;
    return EXIT_SUCCESS;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
