//  Copyright (c) 2011-2012 Thomas Heller
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(HPX_TRAITS_DIRECT_ACTION_HPP)
#define HPX_TRAITS_DIRECT_ACTION_HPP

#include <boost/mpl/bool.hpp>

namespace hpx { namespace traits
{
    ///////////////////////////////////////////////////////////////////////////
    // Customization point for action stack size
    template <typename Action, typename Enable>
    struct direct_action
        : boost::mpl::false_
    {};
}}

#endif

