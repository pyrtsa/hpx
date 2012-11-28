//  Copyright (c) 2007-2012 Hartmut Kaiser
//  Copyright (c)      2011 Bryce Lelbach
//  Copyright (c)      2011 Thomas Heller
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file base_action.hpp

#if !defined(HPX_RUNTIME_ACTIONS_ACTION_HPP)
#define HPX_RUNTIME_ACTIONS_ACTION_HPP

#include <hpx/hpx_fwd.hpp>
#include <hpx/config.hpp>
#include <hpx/util/move.hpp>

#include <hpx/runtime/actions/action_support.hpp>

namespace hpx { namespace actions {
    
    /// Enable hooking into the execution of all actions. This function is 
    /// called for each thread function which is to be returned to the applier.
    ///
    /// This allows to hook into the execution of all actions for a particular
    /// component type.
    template <typename F, F funcptr>
    struct decorate_action
    {
        static HPX_STD_FUNCTION<threads::thread_function_type> 
        call(HPX_STD_FUNCTION<threads::thread_function_type> f,
            naming::address::address_type lva)
        {
            typedef
                typename action_impl<F, funcptr>::component_type
                component_type;
            return
                boost::move(
                    component_type::wrap_action(
                        boost::move(f), lva));
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    /// \tparam Component         component type
    /// \tparam Result            return type
    /// \tparam Arguments         arguments (fusion vector)
    /// \tparam Derived           derived action class
    template <typename F, F funcptr>
    struct action
    {
        typedef F funcptr_type;
        typedef action_impl<F, funcptr> impl;
        typedef typename impl::result_type result_type;
        typedef typename impl::component_type component_type;
        typedef transfer_action<F, funcptr> transfer_action_type;

        typedef void action_tag;

        ///////////////////////////////////////////////////////////////////////
        template <typename Func, typename Arguments>
        static HPX_STD_FUNCTION<threads::thread_function_type>
        construct_continuation_thread_function_void(
            continuation_type cont, BOOST_FWD_REF(Func) func,
            BOOST_FWD_REF(Arguments) args)
        {
            typedef typename boost::remove_reference<Arguments>::type arguments_type;
            return detail::construct_continuation_thread_function_voidN<
                    F, funcptr,
                    boost::fusion::result_of::size<arguments_type>::value>::call(
                cont, boost::forward<Func>(func), boost::forward<Arguments>(args));
        }

        template <typename Func, typename Arguments>
        static HPX_STD_FUNCTION<threads::thread_function_type>
        construct_continuation_thread_function(
            continuation_type cont, BOOST_FWD_REF(Func) func,
            BOOST_FWD_REF(Arguments) args)
        {
            typedef typename boost::remove_reference<Arguments>::type arguments_type;
            return detail::construct_continuation_thread_functionN<
                    F, funcptr,
                    boost::fusion::result_of::size<arguments_type>::value>::call(
                cont, boost::forward<Func>(func), boost::forward<Arguments>(args));
        }

        // bring in all overloads for
        //    construct_continuation_thread_function_void()
        //    construct_continuation_thread_object_function_void()
        //    construct_continuation_thread_function()
        //    construct_continuation_thread_object_function()
        #include <hpx/runtime/actions/construct_continuation_functions.hpp>

        // bring in the definition for all overloads for operator()
        template <typename IdType>
        BOOST_FORCEINLINE typename boost::enable_if<
            boost::mpl::and_<
                boost::mpl::bool_<
                    boost::fusion::result_of::size<
                        typename action_impl<F, funcptr>::arguments_type
                    >::value == 0>,
                boost::is_same<IdType, naming::id_type> >,
            typename traits::promise_local_result<
                typename action_impl<F, funcptr>::result_type
            >::type
        >::type
        operator()(IdType const& id, error_code& ec = throws) const
        {
            return hpx::async(*this, id).get(ec);
        }

        #include <hpx/runtime/actions/define_function_operators.hpp>
    };

}}

#endif
