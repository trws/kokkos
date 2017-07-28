/*
//@HEADER
// ************************************************************************
//
//                        Kokkos v. 2.0
//              Copyright (2014) Sandia Corporation
//
// Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact  H. Carter Edwards (hcedwar@sandia.gov)
//
// ************************************************************************
//@HEADER
*/

#ifndef KOKKOS_CRS_HPP
#define KOKKOS_CRS_HPP

#ifdef DAN
#include <iostream>
#endif

namespace Kokkos {
namespace Experimental {

/// \class Crs
/// \brief Compressed row storage array.
///
/// \tparam DataType The type of stored entries.  If a Crs is
///   used as the graph of a sparse matrix, then this is usually an
///   integer type, the type of the column indices in the sparse
///   matrix.
///
/// \tparam Arg1Type The second template parameter, corresponding
///   either to the Device type (if there are no more template
///   parameters) or to the Layout type (if there is at least one more
///   template parameter).
///
/// \tparam Arg2Type The third template parameter, which if provided
///   corresponds to the Device type.
///
/// \tparam SizeType The type of row offsets.  Usually the default
///   parameter suffices.  However, setting a nondefault value is
///   necessary in some cases, for example, if you want to have a
///   sparse matrices with dimensions (and therefore column indices)
///   that fit in \c int, but want to store more than <tt>INT_MAX</tt>
///   entries in the sparse matrix.
///
/// A row has a range of entries:
/// <ul>
/// <li> <tt> row_map[i0] <= entry < row_map[i0+1] </tt> </li>
/// <li> <tt> 0 <= i1 < row_map[i0+1] - row_map[i0] </tt> </li>
/// <li> <tt> entries( entry ,            i2 , i3 , ... ); </tt> </li>
/// <li> <tt> entries( row_map[i0] + i1 , i2 , i3 , ... ); </tt> </li>
/// </ul>
template< class DataType,
          class Arg1Type,
          class Arg2Type = void,
          typename SizeType = typename ViewTraits<DataType*, Arg1Type, Arg2Type, void >::size_type>
class Crs {
protected:
  typedef ViewTraits<DataType*, Arg1Type, Arg2Type, void> traits;

public:
  typedef DataType                                            data_type;
  typedef typename traits::array_layout                       array_layout;
  typedef typename traits::execution_space                    execution_space;
  typedef typename traits::memory_space                       memory_space;
  typedef typename traits::device_type                        device_type;
  typedef SizeType                                            size_type;

  typedef Crs< DataType , Arg1Type , Arg2Type , SizeType > staticcrsgraph_type;
  typedef Crs< DataType , array_layout , typename traits::host_mirror_space , SizeType > HostMirror;
  typedef View<size_type* , array_layout, device_type> row_map_type;
  typedef View<DataType*  , array_layout, device_type> entries_type;

  row_map_type row_map;
  entries_type entries;

  //! Construct an empty view.
  Crs() : row_map(), entries() {}

  //! Copy constructor (shallow copy).
  Crs(const Crs& rhs) : row_map(rhs.row_map), entries(rhs.entries)
  {}

  template<class EntriesType, class RowMapType>
  Crs(const RowMapType& row_map_, const EntriesType& entries_) : row_map(row_map_), entries(entries_)
  {}

  /** \brief  Assign to a view of the rhs array.
   *          If the old view is the last view
   *          then allocated memory is deallocated.
   */
  Crs& operator= (const Crs& rhs) {
    row_map = rhs.row_map;
    entries = rhs.entries;
    return *this;
  }

  /**  \brief  Destroy this view of the array.
   *           If the last view then allocated memory is deallocated.
   */
  ~Crs() {}

  /**  \brief  Return number of rows in the graph
   */
  KOKKOS_INLINE_FUNCTION
  size_type numRows() const {
    return (row_map.dimension_0 () != 0) ?
      row_map.dimension_0 () - static_cast<size_type> (1) :
      static_cast<size_type> (0);
  }
};

/*--------------------------------------------------------------------------*/

template< class OutCounts,
          class DataType,
          class Arg1Type,
          class Arg2Type,
          class SizeType>
void get_crs_transpose_counts(
    OutCounts& out,
    Crs<DataType, Arg1Type, Arg2Type, SizeType> const& in,
    std::string const& name = "transpose_counts");

template< class OutCounts,
          class InCrs>
typename OutCounts::value_type get_crs_row_map_from_counts(
    OutCounts& out,
    InCrs const& in,
    std::string const& name = "row_map");

template< class DataType,
          class Arg1Type,
          class Arg2Type,
          class SizeType>
void transpose_crs(
    Crs<DataType, Arg1Type, Arg2Type, SizeType>& out,
    Crs<DataType, Arg1Type, Arg2Type, SizeType> const& in);

}} // namespace Kokkos::Experimental

/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/

namespace Kokkos {
namespace Impl {
namespace Experimental {

template <class InCrs, class OutCounts>
class GetCrsTransposeCounts {
 public:
  using execution_space = typename InCrs::execution_space;
  using self_type = GetCrsTransposeCounts<InCrs, OutCounts>;
  using index_type = typename InCrs::size_type;
 private:
  InCrs in;
  OutCounts out;
 public:
  KOKKOS_INLINE_FUNCTION
  void operator()(index_type i) const {
    atomic_increment( &out[in.entries(i)] );
  }
  GetCrsTransposeCounts(InCrs const& arg_in, OutCounts const& arg_out):
    in(arg_in),out(arg_out) {
    using policy_type = RangePolicy<index_type, execution_space>;
    using closure_type = Kokkos::Impl::ParallelFor<self_type, policy_type>;
    const closure_type closure(*this, policy_type(0, index_type(in.entries.size())));
    closure.execute();
    execution_space::fence();
  }
};

template <class InCounts, class OutRowMap>
class CrsRowMapFromCounts {
 public:
  using execution_space = typename InCounts::execution_space;
  using value_type = typename OutRowMap::value_type;
  using index_type = typename InCounts::size_type;
  using last_value_type = Kokkos::View<value_type, execution_space>;
 private:
  InCounts m_in;
  OutRowMap m_out;
  last_value_type m_last_value;
 public:
  KOKKOS_INLINE_FUNCTION
  void operator()(index_type i, value_type& update, bool final_pass) const {
    if (i < m_in.size()) {
      update += m_in(i);
      if (final_pass) m_out(i + 1) = update;
    } else if (final_pass) {
      m_out(0) = 0;
      m_last_value() = update;
    }
  }
  KOKKOS_INLINE_FUNCTION
  void init(value_type& update) const { update = 0; }
  KOKKOS_INLINE_FUNCTION
  void join(volatile value_type& update, const volatile value_type& input) const {
    update += input;
  }
  using self_type = CrsRowMapFromCounts<InCounts, OutRowMap>;
  CrsRowMapFromCounts(InCounts const& arg_in, OutRowMap const& arg_out):
    m_in(arg_in), m_out(arg_out), m_last_value("last_value") {
  }
  value_type execute() {
    using policy_type = RangePolicy<index_type, execution_space>;
    using closure_type = Kokkos::Impl::ParallelScan<self_type, policy_type>;
    closure_type closure(*this, policy_type(0, m_in.size() + 1));
    closure.execute();
    auto last_value = Kokkos::create_mirror_view(m_last_value);
    Kokkos::deep_copy(last_value, m_last_value);
    return last_value();
  }
};

template <class InCrs, class OutCrs>
class FillCrsTransposeEntries {
 public:
  using execution_space = typename InCrs::execution_space;
  using memory_space = typename InCrs::memory_space;
  using value_type = typename OutCrs::entries_type::value_type;
  using index_type = typename InCrs::size_type;
 private:
  using counters_type = View<index_type*, memory_space>;
  InCrs in;
  OutCrs out;
  counters_type counters;
 public:
  KOKKOS_INLINE_FUNCTION
  void operator()(index_type i) const {
    auto begin = in.row_map(i);
    auto end = in.row_map(i + 1);
    for (auto j = begin; j < end; ++j) {
      auto ti = in.entries(j);
      auto tbegin = out.row_map(ti);
      auto tj = atomic_fetch_add( &counters(ti), 1 );
      out.entries( tbegin + tj ) = i;
    }
  }
  using self_type = FillCrsTransposeEntries<InCrs, OutCrs>;
  FillCrsTransposeEntries(InCrs const& arg_in, OutCrs const& arg_out):
    in(arg_in),out(arg_out),
    counters("counters", arg_out.numRows()) {
    using policy_type = RangePolicy<index_type, execution_space>;
    using closure_type = Kokkos::Impl::ParallelFor<self_type, policy_type>;
    const closure_type closure(*this, policy_type(0, index_type(in.numRows())));
    closure.execute();
    execution_space::fence();
  }
};

}}} // namespace Kokkos::Impl::Experimental

/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/

namespace Kokkos {
namespace Experimental {

template< class OutCounts,
          class DataType,
          class Arg1Type,
          class Arg2Type,
          class SizeType>
void get_crs_transpose_counts(
    OutCounts& out,
    Crs<DataType, Arg1Type, Arg2Type, SizeType> const& in,
    std::string const& name) {
  using InCrs = Crs<DataType, Arg1Type, Arg2Type, SizeType>;
  out = OutCounts(name, in.numRows());
  Kokkos::Impl::Experimental::
    GetCrsTransposeCounts<InCrs, OutCounts> functor(in, out);
}

template< class OutRowMap,
          class InCounts>
typename OutRowMap::value_type get_crs_row_map_from_counts(
    OutRowMap& out,
    InCounts const& in,
    std::string const& name) {
  out = OutRowMap(ViewAllocateWithoutInitializing(name), in.size() + 1);
  Kokkos::Impl::Experimental::
    CrsRowMapFromCounts<InCounts, OutRowMap> functor(in, out);
  return functor.execute();
}

template< class DataType,
          class Arg1Type,
          class Arg2Type,
          class SizeType>
void transpose_crs(
    Crs<DataType, Arg1Type, Arg2Type, SizeType>& out,
    Crs<DataType, Arg1Type, Arg2Type, SizeType> const& in)
{
  typedef Crs<DataType, Arg1Type, Arg2Type, SizeType> crs_type ;
  typedef typename crs_type::memory_space             memory_space ;
  typedef View<SizeType*, memory_space>               counts_type ;
  {
  counts_type counts;
  Kokkos::Experimental::get_crs_transpose_counts(counts, in);
  Kokkos::Experimental::get_crs_row_map_from_counts(out.row_map, counts,
      "tranpose_row_map");
  }
  out.entries = decltype(out.entries)("transpose_entries", in.entries.size());
  Kokkos::Impl::Experimental::
    FillCrsTransposeEntries<crs_type, crs_type> entries_functor(in, out);
}

template< class CrsType,
          class Functor>
struct CountAndFill {
  using data_type = typename CrsType::size_type;
  using size_type = typename CrsType::size_type;
  using row_map_type = typename CrsType::row_map_type;
  using entries_type = typename CrsType::entries_type;
  using counts_type = row_map_type;
  CrsType m_crs;
  Functor m_functor;
  counts_type m_counts;
  struct Count {};
  KOKKOS_INLINE_FUNCTION void operator()(Count, size_type i) const {
    m_counts(i) = m_functor(i, nullptr);
  }
  struct Fill {};
  KOKKOS_INLINE_FUNCTION void operator()(Fill, size_type i) const {
    auto j = m_crs.row_map(i);
    data_type* fill = &(m_crs.entries(j));
    m_functor(i, fill);
  }
  using self_type = CountAndFill<CrsType, Functor>;
  CountAndFill(CrsType& crs, size_type nrows, Functor const& f):
    m_crs(crs),
    m_functor(f)
  {
    using execution_space = typename CrsType::execution_space;
    m_counts = counts_type("counts", nrows);
    {
    using count_policy_type = RangePolicy<size_type, execution_space, Count>;
    using count_closure_type =
      Kokkos::Impl::ParallelFor<self_type, count_policy_type>;
    const count_closure_type closure(*this, count_policy_type(0, nrows));
    closure.execute();
    }
#ifdef DAN
    for (int i = 0; i < m_counts.size(); ++i) {
      std::cout << "counts(" << i << ") = " << m_counts(i) << '\n';
    }
#endif
    auto nentries = Kokkos::Experimental::
      get_crs_row_map_from_counts(m_crs.row_map, m_counts);
#ifdef DAN
    for (int i = 0; i < m_crs.row_map.size(); ++i) {
      std::cout << "row_map(" << i << ") = " << m_crs.row_map(i) << '\n';
    }
    std::cout << "nentries " << nentries << '\n';
#endif
    m_counts = counts_type();
    m_crs.entries = entries_type("entries", nentries);
    {
    using fill_policy_type = RangePolicy<size_type, execution_space, Fill>;
    using fill_closure_type =
      Kokkos::Impl::ParallelFor<self_type, fill_policy_type>;
    const fill_closure_type closure(*this, fill_policy_type(0, nrows));
    closure.execute();
    }
#ifdef DAN
    for (int i = 0; i < m_crs.entries.size(); ++i) {
      std::cout << "entries(" << i << ") = " << m_crs.entries(i) << '\n';
    }
#endif
    crs = m_crs;
  }
};

template< class CrsType,
          class Functor>
void count_and_fill_crs(
    CrsType& crs,
    typename CrsType::size_type nrows,
    Functor const& f) {
  Kokkos::Experimental::CountAndFill<CrsType, Functor>(crs, nrows, f);
}

}} // namespace Kokkos::Experimental

#endif /* #define KOKKOS_CRS_HPP */
