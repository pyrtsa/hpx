//  Copyright (c) 2007-2012 Hartmut Kaiser
//  Copyright (c) 2011      Bryce Lelbach
//  Copyright (c) 2011-2012 Thomas Heller
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file component_action.hpp

#if !defined(HPX_RUNTIME_ACTIONS_COMPONENT_ACTION_MAR_26_2008_1054AM)
#define HPX_RUNTIME_ACTIONS_COMPONENT_ACTION_MAR_26_2008_1054AM

#include <cstdlib>
#include <stdexcept>

#include <hpx/hpx_fwd.hpp>
#include <hpx/config.hpp>
#include <hpx/config/bind.hpp>
#include <hpx/exception.hpp>
#include <hpx/runtime/naming/address.hpp>
#include <hpx/runtime/actions/continuation.hpp>
#include <hpx/runtime/actions/action.hpp>
#include <hpx/runtime/components/console_error_sink.hpp>
#include <hpx/util/unused.hpp>
#include <hpx/util/void_cast.hpp>
#include <hpx/util/detail/count_num_args.hpp>

#include <boost/preprocessor/cat.hpp>

#include <hpx/config/warnings_prefix.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace hpx { namespace actions
{
    /// \cond NOINTERNAL
    
    template <typename F, F funcptr>
    struct transfer_action;

    ///////////////////////////////////////////////////////////////////////////
    //  Specialized generic component action types allowing to hold a different
    //  number of arguments
    ///////////////////////////////////////////////////////////////////////////

    // zero argument version
    template <typename Result, typename Component, Result (Component::*funcptr)()>
    struct action_impl<Result (Component::*)(), funcptr>
    {
    public:
        typedef Result (Component::*funcptr_type)();
        typedef  action<funcptr_type, funcptr> action_type;
        typedef Result result_type;
        typedef Component component_type;
        typedef hpx::util::tuple0<> arguments_type;

    protected:
        /// The \a continuation_thread_function will be registered as the thread
        /// function of a thread. It encapsulates the execution of the
        /// original function (given by \a func), while ignoring the return
        /// value.
        template <typename Address>   // dummy template parameter
        BOOST_FORCEINLINE static threads::thread_state_enum
        thread_function(Address lva)
        {
            try {
                LTM_(debug) << "Executing component action("
                            << detail::get_action_name<action_type>()
                            << ") lva(" << reinterpret_cast<void const*>
                                (get_lva<component_type>::call(lva)) << ")";
                (get_lva<component_type>::call(lva)->*funcptr)();  // just call the function
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

    public:
        /// \brief This static \a construct_thread_function allows to construct
        /// a proper thread function for a \a thread without having to
        /// instantiate the \a base_result_action0 type. This is used by the \a
        /// applier in case no continuation has been supplied.
        template <typename Arguments>
        static HPX_STD_FUNCTION<threads::thread_function_type>
        construct_thread_function(naming::address::address_type lva,
            BOOST_FWD_REF(Arguments) /*args*/)
        {
            threads::thread_state_enum (*f)(naming::address::address_type) =
                &thread_function<naming::address::address_type>;

            return boost::move(decorate_action<funcptr_type, funcptr>::call(
                HPX_STD_BIND(f, lva), lva));
        }

        /// \brief This static \a construct_thread_function allows to construct
        /// a proper thread function for a \a thread without having to
        /// instantiate the \a base_result_action0 type. This is used by the \a
        /// applier in case a continuation has been supplied
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
            BOOST_FWD_REF(Arguments))
        {
            LTM_(debug)
                << "action_impl::execute_function: name("
                << detail::get_action_name<action_type>()
                << ") lva(" << reinterpret_cast<void const*>(
                    get_lva<component_type>::call(lva)) << ")";

            return (get_lva<component_type>::call(lva)->*funcptr)();
        }
    };
    
    template <typename Result, typename Component, Result (Component::*funcptr)() const>
    struct action_impl<Result (Component::*)() const, funcptr>
        : action_impl<Result (Component::*)(), funcptr>
    {
        typedef Result (Component::*funcptr_type)() const;
        typedef Result result_type;
        typedef Component const component_type;
        typedef hpx::util::tuple0<> arguments_type;
    };

#if BOOST_WORKAROUND(BOOST_MSVC, == 1600)
    namespace detail
    {
        template <typename Obj, typename Result>
        struct synthesize_const_mf<Obj, Result (*)()>
        {
            typedef Result (Obj::*type)() const;
        };

        template <typename Obj, typename Result>
        struct synthesize_const_mf<Obj, Result (Obj::*)() const>
        {
            typedef Result (Obj::*type)() const;
        };

        template <typename Result>
        typename boost::mpl::identity<Result (*)()>::type
        replicate_type(Result (*p)());
    }
#endif

    ///////////////////////////////////////////////////////////////////////////
    //  zero parameter version, no result value
    template <typename Component, void (Component::*funcptr)()>
    struct action_impl<void (Component::*)(), funcptr>
    {
    public:
        typedef void (Component::*funcptr_type)();
        typedef action<funcptr_type, funcptr> action_type;
        typedef hpx::util::unused_type result_type;
        typedef Component component_type;
        typedef hpx::util::tuple0<> arguments_type;

    protected:
        /// The \a continuation_thread_function will be registered as the thread
        /// function of a thread. It encapsulates the execution of the
        /// original function (given by \a func), while ignoring the return
        /// value.
        template <typename Address>   // dummy template parameter
        BOOST_FORCEINLINE static threads::thread_state_enum
        thread_function(Address lva)
        {
            try {
                LTM_(debug) << "Executing component action("
                            << detail::get_action_name<action_type>()
                            << ") lva(" << reinterpret_cast<void const*>
                                (get_lva<component_type>::call(lva)) << ")";
                (get_lva<component_type>::call(lva)->*funcptr)();  // just call the function
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

    public:
        /// \brief This static \a construct_thread_function allows to construct
        /// a proper thread function for a \a thread without having to
        /// instantiate the base_action0 type. This is used by the \a applier in
        /// case no continuation has been supplied.
        template <typename Arguments>
        static HPX_STD_FUNCTION<threads::thread_function_type>
        construct_thread_function(naming::address::address_type lva,
            BOOST_FWD_REF(Arguments) /*args*/)
        {
            threads::thread_state_enum (*f)(naming::address::address_type) =
                thread_function<naming::address::address_type>;

            return boost::move(decorate_action<funcptr_type, funcptr>::call(
                HPX_STD_BIND(f, lva), lva));
        }

        /// \brief This static \a construct_thread_function allows to construct
        /// a proper thread function for a \a thread without having to
        /// instantiate the base_action0 type. This is used by the \a applier in
        /// case a continuation has been supplied
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
            BOOST_FWD_REF(Arguments))
        {
            LTM_(debug)
                << "direct_action0::execute_function: name("
                << detail::get_action_name<action_type>()
                << ") lva(" << reinterpret_cast<void const*>(
                    get_lva<component_type>::call(lva)) << ")";
            (get_lva<component_type>::call(lva)->*funcptr)();
            return util::unused;
        }
    };
    
    template <typename Component, void (Component::*funcptr)() const>
    struct action_impl<void (Component::*)() const, funcptr>
        : action_impl<void (Component::*)(), funcptr>
    {
        typedef void (Component::*funcptr_type)() const;
        typedef hpx::util::unused_type result_type;
        typedef Component const component_type;
        typedef hpx::util::tuple0<> arguments_type;
    };

    /// \endcond
}}

/// \def HPX_DEFINE_COMPONENT_ACTION(component, func, action_type)
///
/// \brief Registers a non-const member function of a component as an action type
/// with HPX
///
/// The macro \a HPX_DEFINE_COMPONENT_CONST_ACTION can be used to register a
/// non-const member function of a component as an action type named \a action_type.
///
/// The parameter \a component is the type of the component exposing the
/// non-const member function \a func which should be associated with the newly
/// defined action type. The parameter \p action_type is the name of the action
/// type to register with HPX.
///
/// \par Example:
///
/// \code
///       namespace app
///       {
///           // Define a simple component exposing one action 'print_greating'
///           class HPX_COMPONENT_EXPORT server
///             : public hpx::components::simple_component_base<server>
///           {
///               void print_greating ()
///               {
///                   hpx::cout << "Hey, how are you?\n" << hpx::flush;
///               }
///
///               // Component actions need to be declared, this also defines the
///               // type 'print_greating_action' representing the action.
///               HPX_DEFINE_COMPONENT_ACTION(server, print_greating, print_greating_action);
///           };
///       }
/// \endcode
///
/// \note This macro should be used for non-const member functions only. Use
/// the macro \a HPX_DEFINE_COMPONENT_CONST_ACTION for const member functions.
///
/// \note The macro \a HPX_DEFINE_COMPONENT_ACTION can be used with 1 or 
/// 2 arguments. The second argument is optional. The default value for the 
/// second argument (the typename of the defined action) is derived from the 
/// name of the function (as passed as the first argument) by appending '_action'.
/// The second argument can be omitted only if the first argument with an 
/// appended suffix '_action' resolves to a valid, unqualified C++ type name.
#define HPX_DEFINE_COMPONENT_ACTION(...)                                      \
    HPX_DEFINE_COMPONENT_ACTION_(__VA_ARGS__)                                 \
/**/

#define HPX_DEFINE_COMPONENT_ACTION_(...)                                     \
    HPX_UTIL_EXPAND_(BOOST_PP_CAT(                                            \
        HPX_DEFINE_COMPONENT_ACTION_, HPX_UTIL_PP_NARG(__VA_ARGS__)           \
    )(__VA_ARGS__))                                                           \

#define HPX_DEFINE_COMPONENT_ACTION_2(component, func)                        \
    typedef HPX_MAKE_ACTION(component, func)                                  \
        BOOST_PP_CAT(func, _action)                                           \
    /**/
#define HPX_DEFINE_COMPONENT_ACTION_3(component, func, action_type)           \
    typedef HPX_MAKE_ACTION(component, func) action_type                      \
    /**/

/// \def HPX_DEFINE_COMPONENT_CONST_ACTION(component, func, action_type)
///
/// \brief Registers a const member function of a component as an action type
/// with HPX
///
/// The macro \a HPX_DEFINE_COMPONENT_CONST_ACTION can be used to register a
/// const member function of a component as an action type named \a action_type.
///
/// The parameter \a component is the type of the component exposing the const
/// member function \a func which should be associated with the newly defined
/// action type. The parameter \p action_type is the name of the action type to
/// register with HPX.
///
/// \par Example:
///
/// \code
///       namespace app
///       {
///           // Define a simple component exposing one action 'print_greating'
///           class HPX_COMPONENT_EXPORT server
///             : public hpx::components::simple_component_base<server>
///           {
///               void print_greating() const
///               {
///                   hpx::cout << "Hey, how are you?\n" << hpx::flush;
///               }
///
///               // Component actions need to be declared, this also defines the
///               // type 'print_greating_action' representing the action.
///               HPX_DEFINE_COMPONENT_CONST_ACTION(server, print_greating, print_greating_action);
///           };
///       }
/// \endcode
///
/// \note This macro should be used for const member functions only. Use
/// the macro \a HPX_DEFINE_COMPONENT_ACTION for non-const member functions.
///
/// \note The macro \a HPX_DEFINE_COMPONENT_CONST_ACTION can be used with 1 or 
/// 2 arguments. The second argument is optional. The default value for the 
/// second argument (the typename of the defined action) is derived from the 
/// name of the function (as passed as the first argument) by appending '_action'.
/// The second argument can be omitted only if the first argument with an 
/// appended suffix '_action' resolves to a valid, unqualified C++ type name.
#define HPX_DEFINE_COMPONENT_CONST_ACTION(...)                                \
    HPX_DEFINE_COMPONENT_CONST_ACTION_(__VA_ARGS__)                           \
/**/

#define HPX_DEFINE_COMPONENT_CONST_ACTION_(...)                               \
    HPX_UTIL_EXPAND_(BOOST_PP_CAT(                                            \
        HPX_DEFINE_COMPONENT_CONST_ACTION_, HPX_UTIL_PP_NARG(__VA_ARGS__)     \
    )(__VA_ARGS__))                                                           \

#define HPX_DEFINE_COMPONENT_CONST_ACTION_2(component, func)                  \
    typedef HPX_MAKE_CONST_ACTION(component, func)                            \
        BOOST_PP_CAT(func, _action)                                           \
    /**/
#define HPX_DEFINE_COMPONENT_CONST_ACTION_3(component, func, action_type)     \
    typedef HPX_MAKE_CONST_ACTION(component, func) action_type                \
    /**/

/// \cond NOINTERNAL

///////////////////////////////////////////////////////////////////////////////
// same as above, just for template functions
#define HPX_DEFINE_COMPONENT_ACTION_TPL(component, func, name)                \
    typedef HPX_MAKE_ACTION_TPL(component, func) name                         \
    /**/
#define HPX_DEFINE_COMPONENT_CONST_ACTION_TPL(component, func, name)          \
    typedef HPX_MAKE_CONST_ACTION_TPL(component, func) name                   \
    /**/

///////////////////////////////////////////////////////////////////////////////
// bring in the rest of the implementations
#include <hpx/runtime/actions/component_action_implementations.hpp>

///////////////////////////////////////////////////////////////////////////////
// Register the action templates with serialization.
HPX_SERIALIZATION_REGISTER_TEMPLATE(
    (template <typename F, F funcptr>), (hpx::actions::transfer_action<F, funcptr>)
)

/// \endcond

#include <hpx/config/warnings_suffix.hpp>

#endif

