/*******************************************************************************
 *
 * MIT License
 *
 * Copyright 2021-2022 Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 *******************************************************************************/
#ifndef ROCWMMA_OPAQUE_STORE_HPP
#define ROCWMMA_OPAQUE_STORE_HPP

#include "io_traits.hpp"
#include "layout.hpp"
#include "types.hpp"

namespace rocwmma
{

    namespace detail
    {

        template <typename DataT, uint32_t VectorWidth>
        struct amdgcn_opaque_store
        {
            static_assert(VectorWidth > 0, "Vector width must be greater than 0");

            using StoreT = VecT<DataT, VectorWidth>;
            __device__ static inline void
                exec(DataT* dataPtr, StoreT const& data, index_t offset = 0)
            {
                *reinterpret_cast<StoreT*>(&(dataPtr[offset])) = data;
            }
        };

    } // namespace detail

    template <uint32_t BlockDim,
              uint32_t BlockK,
              typename DataT,
              class DataLayout,
              class MatrixLayout,
              uint32_t VectorWidth>
    struct OpaqueStore
    {
        using IOTraits = IOTraits<BlockDim, BlockK, DataT, VectorWidth>;
        struct Traits
        {
            // Raw IO on unpacked register data.
            using Storer = detail::amdgcn_opaque_store<DataT, VectorWidth>;
            using StoreT = typename Storer::StoreT;
            using InputT = VecT<DataT, IOTraits::UnpackedSize>;
        };

        __device__ static void
            exec(DataT* dataPtr, typename Traits::InputT const& data, uint32_t ldm)
        {
            // Arrange wave threads to starting matrix layout offsets.
            auto baseOffset = MatrixLayout::baseOffset();

            auto it = data.template cbegin<Traits::StoreT::size()>();
            static_assert(decltype(it)::range() == IOTraits::IOCount,
                          "IOCount inconsistent with iterator range");

            // Loop through entire block
#pragma unroll
            for(auto i = 0; i < IOTraits::IOCount; ++i)
            {
                Traits::Storer::exec(dataPtr, *it, DataLayout::fromMatrixCoord(baseOffset, ldm));
                baseOffset += MatrixLayout::incrementalOffset(it.index());
                it++;
            }
        }
    };

} // namespace rocwmma

#endif // ROCWMMA_OPAQUE_STORE_HPP