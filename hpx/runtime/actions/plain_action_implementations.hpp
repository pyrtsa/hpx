//  Copyright (c) 2007-2012 Hartmut Kaiser
//  Copyright (c) 2011      Bryce Lelbach
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PP_IS_ITERATING

#if !defined(HPX_RUNTIME_ACTIONS_PLAIN_ACTION_IMPLEMENTATIONS_NOV_14_2008_0811PM)
#define HPX_RUNTIME_ACTIONS_PLAIN_ACTION_IMPLEMENTATIONS_NOV_14_2008_0811PM

#include <hpx/config/forceinline.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/repeat.hpp>
#include <boost/preprocessor/iterate.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/repetition/enum_binary_params.hpp>

#if !defined(HPX_USE_PREPROCESSOR_LIMIT_EXPANSION)
#  include <hpx/runtime/actions/preprocessed/plain_action_implementations.hpp>
#else

#if defined(__WAVE__) && defined(HPX_CREATE_PREPROCESSED_FILES)
#  pragma wave option(preserve: 1, line: 0, output: "preprocessed/plain_action_implementations_" HPX_LIMIT_STR ".hpp")
#endif

#define BOOST_PP_ITERATION_PARAMS_1                                           \
    (3, (1, HPX_ACTION_ARGUMENT_LIMIT,                                        \
    "hpx/runtime/actions/plain_action_implementations.hpp"))                  \
    /**/

#include BOOST_PP_ITERATE()

#if defined(__WAVE__) && defined (HPX_CREATE_PREPROCESSED_FILES)
#  pragma wave option(output: null)
#endif

#endif // !defined(HPX_USE_PREPROCESSOR_LIMIT_EXPANSION)

#endif

///////////////////////////////////////////////////////////////////////////////
//  Preprocessor vertical repetition code
///////////////////////////////////////////////////////////////////////////////
#else // defined(BOOST_PP_IS_ITERATING)

#define N BOOST_PP_ITERATION()

#define HPX_ACTION_DIRECT_ARGUMENT(z, n, data)                                \
    BOOST_PP_COMMA_IF(n)                                                      \
    util::detail::move_if_no_ref<                                             \
        typename util::detail::remove_reference<Arguments>::type::            \
            BOOST_PP_CAT(member_type, n)>::call(data. BOOST_PP_CAT(a, n))     \
    /**/
#define HPX_REMOVE_QUALIFIERS(z, n, data)                                     \
        BOOST_PP_COMMA_IF(n)                                                  \
        typename detail::remove_qualifiers<BOOST_PP_CAT(T, n)>::type          \
    /**/

namespace hpx { namespace actions
{
    ///////////////////////////////////////////////////////////////////////////
    //  N parameter version, with result
    template <
        typename Result,
        BOOST_PP_ENUM_PARAMS(N, typename T),
        void (*funcptr)(BOOST_PP_ENUM_PARAMS(N, T))>
    class action_impl<Result (*)(BOOST_PP_ENUM_PARAMS(N, T)), funcptr>
    {
    public:
        typedef Result (*funcptr_type)(BOOST_PP_ENUM_PARAMS(N, T));
        typedef
            components::server::plain_function<action<funcptr_type, funcptr> >
            component_type;
        typedef Result result_type;
        typedef
            BOOST_PP_CAT(hpx::util::tuple, N)<BOOST_PP_REPEAT(N, HPX_REMOVE_QUALIFIERS, _)>
            arguments_type;

    protected:
        /// The \a thread_function will be registered as the thread
        /// function of a thread. It encapsulates the execution of the
        /// original function (given by \a func).
        struct thread_function
        {
            typedef threads::thread_state_enum result_type;

            template <BOOST_PP_ENUM_PARAMS(N, typename Arg)>
            BOOST_FORCEINLINE result_type operator()(
                HPX_ENUM_FWD_ARGS(N, Arg, arg)) const
            {
                try {
                    LTM_(debug) << "Executing plain action("
                                << detail::get_action_name<funcptr_type, funcptr>()
                                << ").";

                    // The arguments are moved here. This function is called from a
                    // bound functor. In order to do true perfect forwarding in an
                    // asynchronous operation. These bound variables must be moved
                    // out of the bound object.

                    // call the function, ignoring the return value
                    funcptr(HPX_ENUM_MOVE_ARGS(N, arg));
                }
                catch (hpx::exception const& e) {
                    if (e.get_error() != hpx::thread_interrupted) {
                        LTM_(error)
                            << "Unhandled exception while executing plain action("
                            << detail::get_action_name<funcptr_type, funcptr>()
                            << "): " << e.what();

                        // report this error to the console in any case
                        hpx::report_error(boost::current_exception());
                    }
                }

                // Verify that there are no more registered locks for this
                // OS-thread. This will throw if there are still any locks
                // held.
                util::force_error_on_lock();
                return threads::terminated;
            }
        };

    public:
        // This static construct_thread_function allows to construct
        // a proper thread function for a thread without having to
        // instantiate the base_result_actionN type. This is used by the applier in
        // case no continuation has been supplied.
        template <typename Arguments>
        static HPX_STD_FUNCTION<threads::thread_function_type>
        construct_thread_function(naming::address::address_type lva,
            BOOST_FWD_REF(Arguments) args)
        {
            return boost::move(decorate_action<funcptr_type, funcptr>::call(
                HPX_STD_BIND(thread_function(),
                    BOOST_PP_REPEAT(N, HPX_ACTION_DIRECT_ARGUMENT, args)), lva));
        }

        // This static construct_thread_function allows to construct
        // a proper thread function for a thread without having to
        // instantiate the base_result_actionN type. This is used by the applier in
        // case a continuation has been supplied
        template <typename Arguments>
        static HPX_STD_FUNCTION<threads::thread_function_type>
        construct_thread_function(continuation_type& cont,
            naming::address::address_type lva, BOOST_FWD_REF(Arguments) args)
        {
            return boost::move(decorate_action<funcptr_type, funcptr>::call(
                action<funcptr_type, funcptr>::construct_continuation_thread_function(
                    cont, funcptr, boost::forward<Arguments>(args)), lva));
        }

        template <typename Arguments>
        BOOST_FORCEINLINE static result_type
        execute_function(naming::address::address_type lva,
            BOOST_FWD_REF(Arguments) args)
        {
            LTM_(debug)
                << "plain_direct_result_action" << N
                << "::execute_function name("
                << detail::get_action_name<funcptr_type, funcptr>() << ")";

            return funcptr(BOOST_PP_REPEAT(N, HPX_ACTION_DIRECT_ARGUMENT, args));
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    //  N parameter version, no result type
    template <
        BOOST_PP_ENUM_PARAMS(N, typename T),
        void (*funcptr)(BOOST_PP_ENUM_PARAMS(N, T))>
    class action_impl<void (*)(BOOST_PP_ENUM_PARAMS(N, T)), funcptr>
    {
    public:
        typedef void (*funcptr_type)(BOOST_PP_ENUM_PARAMS(N, T));
        typedef
            components::server::plain_function<action<funcptr_type, funcptr> >
            component_type;
        typedef hpx::util::unused_type result_type;
        typedef
            BOOST_PP_CAT(hpx::util::tuple, N)<BOOST_PP_REPEAT(N, HPX_REMOVE_QUALIFIERS, _)>
            arguments_type;

    protected:
        /// The \a thread_function will be registered as the thread
        /// function of a thread. It encapsulates the execution of the
        /// original function (given by \a func).
        struct thread_function
        {
            typedef threads::thread_state_enum result_type;

            template <BOOST_PP_ENUM_PARAMS(N, typename Arg)>
            BOOST_FORCEINLINE result_type operator()(
                HPX_ENUM_FWD_ARGS(N, Arg, arg)) const
            {
                try {
                    LTM_(debug) << "Executing plain action("
                                << detail::get_action_name<funcptr_type, funcptr>()
                                << ").";

                    // The arguments are moved here. This function is called from a
                    // bound functor. In order to do true perfect forwarding in an
                    // asynchronous operation. These bound variables must be moved
                    // out of the bound object.

                    // call the function, ignoring the return value
                    funcptr(HPX_ENUM_MOVE_ARGS(N, arg));
                }
                catch (hpx::exception const& e) {
                    if (e.get_error() != hpx::thread_interrupted) {
                        LTM_(error)
                            << "Unhandled exception while executing plain action("
                            << detail::get_action_name<funcptr_type, funcptr>()
                            << "): " << e.what();

                        // report this error to the console in any case
                        hpx::report_error(boost::current_exception());
                    }
                }

                // Verify that there are no more registered locks for this
                // OS-thread. This will throw if there are still any locks
                // held.
                util::force_error_on_lock();
                return threads::terminated;
            }
        };

    public:
        // This static construct_thread_function allows to construct
        // a proper thread function for a thread without having to
        // instantiate the base_actionN type. This is used by the applier in
        // case no continuation has been supplied.
        template <typename Arguments>
        static HPX_STD_FUNCTION<threads::thread_function_type>
        construct_thread_function(naming::address::address_type lva,
            BOOST_FWD_REF(Arguments) args)
        {
            return boost::move(decorate_action<funcptr_type, funcptr>::call(
                HPX_STD_BIND(thread_function(),
                    BOOST_PP_REPEAT(N, HPX_ACTION_DIRECT_ARGUMENT, args)), lva));
        }

        // This static construct_thread_function allows to construct
        // a proper thread function for a thread without having to
        // instantiate the base_actionN type. This is used by the applier in
        // case a continuation has been supplied
        template <typename Arguments>
        static HPX_STD_FUNCTION<threads::thread_function_type>
        construct_thread_function(continuation_type& cont,
            naming::address::address_type lva, BOOST_FWD_REF(Arguments) args)
        {
            return boost::move(decorate_action<funcptr_type, funcptr>::call(
                action<funcptr_type, funcptr>::construct_continuation_thread_function_void(
                    cont, funcptr, boost::forward<Arguments>(args)), lva));
        }

        template <typename Arguments>
        BOOST_FORCEINLINE static util::unused_type
        execute_function(naming::address::address_type lva,
            BOOST_FWD_REF(Arguments) args)
        {
            LTM_(debug)
                << "plain_direct_action" << N
                << "::execute_function name("
                << detail::get_action_name<funcptr_type, funcptr>() << ")";

            funcptr(BOOST_PP_REPEAT(N, HPX_ACTION_DIRECT_ARGUMENT, args));
            return util::unused;
        }
    };
}}

// Disabling the guid initialization stuff for plain actions
namespace hpx { namespace traits
{
    template <BOOST_PP_ENUM_PARAMS(N, typename Arg),
        void (*F)(BOOST_PP_ENUM_PARAMS(N, Arg)), 
        typename Enable>
    struct needs_guid_initialization<
            hpx::actions::transfer_action<
                void(*)(BOOST_PP_ENUM_PARAMS(N, Arg)), F>, Enable>
      : boost::mpl::false_
    {};

    template <typename R, BOOST_PP_ENUM_PARAMS(N, typename Arg),
        R(*F)(BOOST_PP_ENUM_PARAMS(N, Arg)),
        typename Enable>
    struct needs_guid_initialization<
            hpx::actions::transfer_action<
                R(*)(BOOST_PP_ENUM_PARAMS(N, Arg)), F>, Enable>
      : boost::mpl::false_
    {};
}}

///////////////////////////////////////////////////////////////////////////////
#undef HPX_REMOVE_QUALIFIERS
#undef HPX_ACTION_DIRECT_ARGUMENT
#undef N

#endif

