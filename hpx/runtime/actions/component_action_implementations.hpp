//  Copyright (c) 2007-2012 Hartmut Kaiser
//  Copyright (c) 2011      Bryce Lelbach
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PP_IS_ITERATING

#if !defined(HPX_RUNTIME_ACTIONS_ACTION_IMPLEMENTATIONS_MAY_20_2008_1104AM)
#define HPX_RUNTIME_ACTIONS_ACTION_IMPLEMENTATIONS_MAY_20_2008_1104AM

#include <hpx/config/forceinline.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/repeat.hpp>
#include <boost/preprocessor/iterate.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/repetition/enum_binary_params.hpp>

// generate platform specific code
#if BOOST_WORKAROUND(BOOST_MSVC, == 1600)
#define BOOST_PP_ITERATION_PARAMS_1                                           \
    (4, (1, HPX_ACTION_ARGUMENT_LIMIT,                                        \
    "hpx/runtime/actions/component_action_implementations.hpp", 1))           \
    /**/

#include BOOST_PP_ITERATE()
#endif

// now generate the rest, which is platform independent
#if !defined(HPX_USE_PREPROCESSOR_LIMIT_EXPANSION)
#  include <hpx/runtime/actions/preprocessed/component_action_implementations.hpp>
#else

#if defined(__WAVE__) && defined(HPX_CREATE_PREPROCESSED_FILES)
#  pragma wave option(preserve: 1, line: 0, output: "preprocessed/component_action_implementations_" HPX_LIMIT_STR ".hpp")
#endif

#define BOOST_PP_ITERATION_PARAMS_1                                           \
    (4, (1, HPX_ACTION_ARGUMENT_LIMIT,                                        \
    "hpx/runtime/actions/component_action_implementations.hpp", 2))           \
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

#if BOOST_PP_ITERATION_FLAGS() == 1

namespace hpx { namespace actions { namespace detail
{
    template <typename Obj, typename Result,
        BOOST_PP_ENUM_PARAMS(N, typename T)>
    struct synthesize_const_mf<Obj,
        Result (*)(BOOST_PP_ENUM_PARAMS(N, T))>
    {
        typedef Result (Obj::*type)(BOOST_PP_ENUM_PARAMS(N, T)) const;
    };

    template <typename Obj, typename Result,
        BOOST_PP_ENUM_PARAMS(N, typename T)>
    struct synthesize_const_mf<Obj,
        Result (Obj::*)(BOOST_PP_ENUM_PARAMS(N, T)) const>
    {
        typedef Result (Obj::*type)(BOOST_PP_ENUM_PARAMS(N, T)) const;
    };

    template <typename Result, BOOST_PP_ENUM_PARAMS(N, typename T)>
    typename boost::mpl::identity<Result (*)(BOOST_PP_ENUM_PARAMS(N, T))>::type
    replicate_type(Result (*p)(BOOST_PP_ENUM_PARAMS(N, T)));
}}}

#endif

#if BOOST_PP_ITERATION_FLAGS() == 2

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
    template <typename Component, typename Result,
        BOOST_PP_ENUM_PARAMS(N, typename T),
        Result (Component::*funcptr)(BOOST_PP_ENUM_PARAMS(N, T))>
    struct action_impl<Result (Component::*)(BOOST_PP_ENUM_PARAMS(N, T)), funcptr>
    {
        typedef Result (Component::*funcptr_type)(BOOST_PP_ENUM_PARAMS(N, T));
        typedef action<funcptr_type, funcptr> action_type;
        typedef Component component_type;
        typedef Result result_type;
        typedef BOOST_PP_CAT(hpx::util::tuple, N)<
            BOOST_PP_REPEAT(N, HPX_REMOVE_QUALIFIERS, _)> arguments_type;

    protected:
        /// The \a thread_function will be registered as the thread
        /// function of a thread. It encapsulates the execution of the
        /// original function (given by \a func).
        struct thread_function
        {
            typedef threads::thread_state_enum result_type;

            template <BOOST_PP_ENUM_PARAMS(N, typename Arg)>
            BOOST_FORCEINLINE result_type operator()(
                naming::address::address_type lva,
                HPX_ENUM_FWD_ARGS(N, Arg, arg)) const
            {
                try {
                    LTM_(debug) << "Executing component action("
                                << detail::get_action_name<action_type>()
                                << ") lva(" << reinterpret_cast<void const*>
                                    (get_lva<component_type>::call(lva)) << ")";
                    // The arguments are moved here. This function is called from a
                    // bound functor. In order to do true perfect forwarding in an
                    // asynchronous operation. These bound variables must be moved
                    // out of the bound object.
                    (get_lva<component_type>::call(lva)->*funcptr)(
                        HPX_ENUM_MOVE_ARGS(N, arg));
                }
                catch (hpx::exception const& e) {
                    if (e.get_error() != hpx::thread_interrupted) {
                        LTM_(error)
                            << "Unhandled exception while executing component action("
                            << detail::get_action_name<action_type>()
                            << ") lva(" << reinterpret_cast<void const*>
                                (get_lva<component_type>::call(lva)) << "): " << e.what();

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
        // instantiate the base_result_actionN type. This is used by the
        // applier in case no continuation has been supplied.
        template <typename Arguments>
        static HPX_STD_FUNCTION<threads::thread_function_type>
        construct_thread_function(naming::address::address_type lva,
            BOOST_FWD_REF(Arguments) args)
        {
            return boost::move(decorate_action<funcptr_type, funcptr>::call(
                HPX_STD_BIND(thread_function(),
                    lva, BOOST_PP_REPEAT(N, HPX_ACTION_DIRECT_ARGUMENT, args)), lva));
        }

        // This static construct_thread_function allows to construct
        // a proper thread function for a thread without having to
        // instantiate the base_result_actionN type. This is used by the
        // applier in case a continuation has been supplied
        template <typename Arguments>
        static HPX_STD_FUNCTION<threads::thread_function_type>
        construct_thread_function(continuation_type& cont,
            naming::address::address_type lva, BOOST_FWD_REF(Arguments) args)
        {
            return boost::move(decorate_action<funcptr_type, funcptr>::call(
                    action<funcptr_type, funcptr>::construct_continuation_thread_object_function(
                        cont, funcptr, get_lva<component_type>::call(lva),
                        boost::forward<Arguments>(args)), lva));
        }

        template <typename Arguments>
        BOOST_FORCEINLINE static result_type
        execute_function(naming::address::address_type lva,
            BOOST_FWD_REF(Arguments) args)
        {
            LTM_(debug)
                << "action_impl::execute_function name("
                << detail::get_action_name<action_type>()
                << ") lva(" << reinterpret_cast<void const*>(
                    get_lva<component_type>::call(lva)) << ")";

            return (get_lva<component_type>::call(lva)->*funcptr)(
                BOOST_PP_REPEAT(N, HPX_ACTION_DIRECT_ARGUMENT, args));
        }
    };
    template <typename Component, typename Result,
        BOOST_PP_ENUM_PARAMS(N, typename T),
        Result (Component::*funcptr)(BOOST_PP_ENUM_PARAMS(N, T)) const>
    struct action_impl<
        Result (Component::*)(BOOST_PP_ENUM_PARAMS(N, T)) const, funcptr>
      : action_impl<Result (Component::*)(BOOST_PP_ENUM_PARAMS(N, T)), funcptr>
    {
        typedef Result (Component::*funcptr_type)(BOOST_PP_ENUM_PARAMS(N, T)) const;
        typedef Component const component_type;
        typedef Result result_type;
        typedef BOOST_PP_CAT(hpx::util::tuple, N)<
            BOOST_PP_REPEAT(N, HPX_REMOVE_QUALIFIERS, _)> arguments_type;
    };

    ///////////////////////////////////////////////////////////////////////////
    //  N parameter version, no result type
    template <typename Component, BOOST_PP_ENUM_PARAMS(N, typename T),
        void (Component::*funcptr)(BOOST_PP_ENUM_PARAMS(N, T))>
    struct action_impl<void (Component::*)(BOOST_PP_ENUM_PARAMS(N, T)), funcptr>
    {
    public:
        typedef void (Component::*funcptr_type)(BOOST_PP_ENUM_PARAMS(N, T));
        typedef action<funcptr_type, funcptr> action_type;
        typedef Component component_type;
        typedef util::unused_type result_type;
        typedef BOOST_PP_CAT(hpx::util::tuple, N)<
            BOOST_PP_REPEAT(N, HPX_REMOVE_QUALIFIERS, _)> arguments_type;

    protected:
        /// The \a thread_function will be registered as the thread
        /// function of a thread. It encapsulates the execution of the
        /// original function (given by \a func).
        struct thread_function
        {
            typedef threads::thread_state_enum result_type;

            template <BOOST_PP_ENUM_PARAMS(N, typename Arg)>
            BOOST_FORCEINLINE result_type operator()(
                naming::address::address_type lva,
                HPX_ENUM_FWD_ARGS(N, Arg, arg)) const
            {
                try {
                    LTM_(debug) << "Executing component action("
                                << detail::get_action_name<action_type>()
                                << ") lva(" << reinterpret_cast<void const*>
                                    (get_lva<component_type>::call(lva)) << ")";
                    // The arguments are moved here. This function is called from a
                    // bound functor. In order to do true perfect forwarding in an
                    // asynchronous operation. These bound variables must be moved
                    // out of the bound object.
                    (get_lva<component_type>::call(lva)->*funcptr)(
                        HPX_ENUM_MOVE_ARGS(N, arg));
                }
                catch (hpx::exception const& e) {
                    if (e.get_error() != hpx::thread_interrupted) {
                        LTM_(error)
                            << "Unhandled exception while executing component action("
                            << detail::get_action_name<action_type>()
                            << ") lva(" << reinterpret_cast<void const*>
                                (get_lva<component_type>::call(lva)) << "): " << e.what();

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
            // we need to assign the address of the thread function to a
            // variable to  help the compiler to deduce the function type
            return boost::move(decorate_action<funcptr_type, funcptr>::call(
                HPX_STD_BIND(thread_function(), lva,
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
                    action<funcptr_type, funcptr>::construct_continuation_thread_object_function_void(
                        cont, funcptr, get_lva<component_type>::call(lva),
                        boost::forward<Arguments>(args)), lva));
        }

        template <typename Arguments>
        BOOST_FORCEINLINE static util::unused_type
        execute_function(naming::address::address_type lva,
            BOOST_FWD_REF(Arguments) args)
        {
            LTM_(debug)
                << "action_impl::execute_function name("
                << detail::get_action_name<action_type>()
                << ") lva(" << reinterpret_cast<void const*>(
                    get_lva<component_type>::call(lva)) << ")";

            (get_lva<component_type>::call(lva)->*funcptr)(
                BOOST_PP_REPEAT(N, HPX_ACTION_DIRECT_ARGUMENT, args));
            return util::unused;
        }
    };
    
    template <typename Component, BOOST_PP_ENUM_PARAMS(N, typename T),
        void (Component::*funcptr)(BOOST_PP_ENUM_PARAMS(N, T)) const>
    struct action_impl<void (Component::*)(BOOST_PP_ENUM_PARAMS(N, T)) const, funcptr>
      : action_impl<void (Component::*)(BOOST_PP_ENUM_PARAMS(N, T)), funcptr>
    {
        typedef void (Component::*funcptr_type)(BOOST_PP_ENUM_PARAMS(N, T)) const;
        typedef Component const component_type;
        typedef util::unused_type result_type;
        typedef BOOST_PP_CAT(hpx::util::tuple, N)<
            BOOST_PP_REPEAT(N, HPX_REMOVE_QUALIFIERS, _)> arguments_type;
    };
    
}}

///////////////////////////////////////////////////////////////////////////////
#undef HPX_REMOVE_QUALIFIERS
#undef HPX_ACTION_DIRECT_ARGUMENT
// #undef HPX_ACTION_ARGUMENT

#endif // #if BOOST_PP_ITERATION_FLAGS() == 2

#undef N

#endif

