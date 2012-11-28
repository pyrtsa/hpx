//  Copyright (c) 2007-2012 Hartmut Kaiser
//  Copyright (c)      2011 Bryce Lelbach
//  Copyright (c)      2011 Thomas Heller
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file action_support.hpp

#if !defined(HPX_RUNTIME_ACTIONS_ACTION_SUPPORT_NOV_14_2008_0711PM)
#define HPX_RUNTIME_ACTIONS_ACTION_SUPPORT_NOV_14_2008_0711PM

#include <hpx/hpx_fwd.hpp>
#include <hpx/config.hpp>
#include <hpx/util/move.hpp>
#include <hpx/traits/action_priority.hpp>
#include <hpx/traits/action_stacksize.hpp>
#include <hpx/traits/direct_action.hpp>
#include <hpx/traits/type_size.hpp>

#include <boost/version.hpp>
#include <boost/fusion/include/vector.hpp>
#include <boost/fusion/include/at.hpp>
#include <boost/fusion/include/size.hpp>
#include <boost/fusion/include/for_each.hpp>
#include <boost/fusion/include/at_c.hpp>
#include <boost/ref.hpp>
#include <boost/foreach.hpp>
#include <boost/serialization/access.hpp>
#include <boost/serialization/export.hpp>
#include <boost/serialization/extended_type_info.hpp>
#include <boost/serialization/extended_type_info_typeid.hpp>
#include <boost/type_traits/remove_const.hpp>
#include <boost/type_traits/remove_reference.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/type_traits/is_convertible.hpp>
#include <boost/mpl/if.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/mpl/and.hpp>
#include <boost/mpl/identity.hpp>
#if defined(BOOST_NO_DECLTYPE)
#  include <boost/typeof/typeof.hpp>
#endif
#include <boost/utility/enable_if.hpp>
#include <boost/preprocessor/cat.hpp>

#include <hpx/traits/needs_guid_initialization.hpp>
#include <hpx/runtime/get_lva.hpp>
#include <hpx/runtime/threads/thread_helpers.hpp>
#include <hpx/runtime/threads/thread_init_data.hpp>
#include <hpx/runtime/actions/continuation.hpp>
#include <hpx/runtime/actions/base_action.hpp>
#include <hpx/util/serialize_sequence.hpp>
#include <hpx/util/serialize_exception.hpp>
#include <hpx/util/demangle_helper.hpp>
#include <hpx/util/base_object.hpp>
#include <hpx/util/void_cast.hpp>
#include <hpx/util/register_locks.hpp>
#include <hpx/util/detail/count_num_args.hpp>
#include <hpx/config/bind.hpp>
#include <hpx/config/tuple.hpp>
#include <hpx/config/function.hpp>

#include <hpx/config/warnings_prefix.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace hpx { namespace actions
{
    /// \cond NOINTERNAL
    template <typename F, F funcptr>
    struct transfer_action;
    
    template <typename F, F funcptr>
    struct action;

    ///////////////////////////////////////////////////////////////////////////
    namespace detail
    {
        // Helper template meta function removing any 'const' qualifier or
        // reference from the given type (i.e. const& T --> T)
        template <typename T>
        struct remove_qualifiers
        {
            typedef typename hpx::util::detail::remove_reference<T>::type no_ref_type;
            typedef typename boost::remove_const<no_ref_type>::type type;
        };

        template <typename Action>
        char const* get_action_name()
        {
            /// If you encounter this assert while compiling code, that means that
            /// you have a HPX_REGISTER_ACTION macro somewhere in a source file,
            /// but the header in which the action is defined misses a
            /// HPX_REGISTER_ACTION_DECLARATION
            BOOST_MPL_ASSERT_MSG(
                traits::needs_guid_initialization<Action>::value
              , HPX_REGISTER_ACTION_DECLARATION_MISSING
              , (Action)
            );
            return util::type_id<Action>::typeid_.type_id();
        }

        ///////////////////////////////////////////////////////////////////////
        // Figure out what priority the action has to be be associated with
        // A dynamically specified default priority results in using the static
        // Priority.
        template <threads::thread_priority Priority>
        struct thread_priority
        {
            static threads::thread_priority
            call(threads::thread_priority priority)
            {
                if (priority == threads::thread_priority_default)
                    return Priority;
                return priority;
            }
        };

        // If the static Priority is default, a dynamically specified default
        // priority results in using the normal priority.
        template <>
        struct thread_priority<threads::thread_priority_default>
        {
            static threads::thread_priority
            call(threads::thread_priority priority)
            {
                if (priority == threads::thread_priority_default)
                    return threads::thread_priority_normal;
                return priority;
            }
        };

        ///////////////////////////////////////////////////////////////////////
        // Figure out what stacksize the action has to be be associated with
        // A dynamically specified default stacksize results in using the static
        // Stacksize.
        template <threads::thread_stacksize Stacksize>
        struct thread_stacksize
        {
            static threads::thread_stacksize
            call(threads::thread_stacksize stacksize)
            {
                if (stacksize == threads::thread_stacksize_default)
                    return Stacksize;
                return stacksize;
            }
        };

        // If the static Stacksize is default, a dynamically specified default
        // stacksize results in using the normal stacksize.
        template <>
        struct thread_stacksize<threads::thread_stacksize_default>
        {
            static threads::thread_stacksize
            call(threads::thread_stacksize stacksize)
            {
                if (stacksize == threads::thread_stacksize_default)
                    return threads::thread_stacksize_minimal;
                return stacksize;
            }
        };

        template <typename Action,
            typename DirectExecute = typename hpx::traits::direct_action<Action>::type>
        struct get_action_type
        {
            static base_action::action_type call()
            {
                return base_action::plain_action;
            }
        };
        
        template <typename Action>
        struct get_action_type<Action, boost::mpl::true_>
        {
            static base_action::action_type call()
            {
                return base_action::direct_action;
            }
        };
    }

    template <typename F, F funcptr>
    struct action_impl;


    #include <hpx/runtime/actions/construct_continuation_function_objects.hpp>

// older compilers require BOOST_TYPEOF, newer compilers have decltype()
#if defined(HPX_HAVE_CXX11_DECLTYPE)
#  define HPX_TYPEOF(x)       decltype(x)
#  define HPX_TYPEOF_TPL(x)   decltype(x)
#elif HPX_GCC_VERSION <= 40400
#  define HPX_TYPEOF(x)       __typeof__(x)
#  define HPX_TYPEOF_TPL(x)   __typeof__(x)
#else
#  define HPX_TYPEOF(x)       BOOST_TYPEOF(x)
#  define HPX_TYPEOF_TPL(x)   BOOST_TYPEOF_TPL(x)
#endif

#if BOOST_WORKAROUND(BOOST_MSVC, == 1600)
    // workarounds for VC2010
    #define HPX_MAKE_FUNCTION_TYPE_2(component, f)                            \
        HPX_TYPEOF(HPX_UTIL_STRIP(component)::HPX_UTIL_STRIP(f))              \
                HPX_UTIL_STRIP(component)::*                                  \
    /**/
    
    #define HPX_MAKE_FUNCTION_TYPE_TPL_2(component, f)                        \
        HPX_TYPEOF(HPX_UTIL_STRIP(component)::HPX_UTIL_STRIP(f))              \
                HPX_UTIL_STRIP(component)::*                                  \
    /**/
    
    namespace detail
    {
        template <typename Obj, typename F>
        struct synthesize_const_mf;

        template <typename F> F replicate_type(F);
    }

    #define HPX_MAKE_CONST_FUNCTION_TYPE_2(component, f)                      \
        hpx::actions::detail::synthesize_const_mf<                            \
            HPX_UTIL_STRIP(component), HPX_TYPEOF(                            \
                hpx::actions::detail::replicate_type(                         \
                    &HPX_UTIL_STRIP(component)::HPX_UTIL_STRIP(f))            \
            )                                                                 \
        >::type                                                               \
    /**/
    
    #define HPX_MAKE_CONST_FUNCTION_TYPE_TPL_2(component, f)                  \
        typename hpx::actions::detail::synthesize_const_mf<                   \
            HPX_UTIL_STRIP(component), HPX_TYPEOF_TPL(                        \
                hpx::actions::detail::replicate_type(                         \
                    &HPX_UTIL_STRIP(component)::HPX_UTIL_STRIP(f))            \
            )                                                                 \
        >::type                                                               \
    /**/
#else
    // the implementation on conforming compilers is almost trivial
    #define HPX_MAKE_FUNCTION_TYPE_2(component, f)                            \
        HPX_TYPEOF(&HPX_UTIL_STRIP(component)::HPX_UTIL_STRIP(f))             \
    /**/
    #define HPX_MAKE_FUNCTION_TYPE_TPL_2(component, f)                        \
        HPX_TYPEOF_TPL(&HPX_UTIL_STRIP(component)::HPX_UTIL_STRIP(f))         \
    /**/
    #define HPX_MAKE_CONST_FUNCTION_TYPE_2(component, f)                      \
        HPX_TYPEOF(&HPX_UTIL_STRIP(component)::HPX_UTIL_STRIP(f))             \
    /**/
    #define HPX_MAKE_CONST_FUNCTION_TYPE_TPL_2(component, f)                  \
        HPX_TYPEOF_TPL(&HPX_UTIL_STRIP(component)::HPX_UTIL_STRIP(f))         \
    /**/
#endif

    #define HPX_MAKE_FUNCTION_TYPE_1(f)                                       \
        HPX_TYPEOF(&HPX_UTIL_STRIP(f))                                        \
    /**/
    #define HPX_MAKE_FUNCTION_TYPE_TPL_1(f)                                   \
        HPX_TYPEOF_TPL(&HPX_UTIL_STRIP(f))                                    \
    /**/

    #define HPX_MAKE_FUNCTION_TYPE(...)                                       \
        HPX_UTIL_EXPAND_(BOOST_PP_CAT(                                        \
            HPX_MAKE_FUNCTION_TYPE_, HPX_UTIL_PP_NARG(__VA_ARGS__)            \
        )(__VA_ARGS__))                                                       \
    /**/

    #define HPX_MAKE_FUNCTION_TYPE_TPL(...)                                   \
        HPX_UTIL_EXPAND_(BOOST_PP_CAT(                                        \
            HPX_MAKE_FUNCTION_TYPE_TPL_, HPX_UTIL_PP_NARG(__VA_ARGS__)        \
        )(__VA_ARGS__))                                                       \
    /**/

    #define HPX_MAKE_FUNCTION_PTR_1(f)                                        \
        &HPX_UTIL_STRIP(f)                                                    \
    /**/
    #define HPX_MAKE_FUNCTION_PTR_2(component, f)                             \
        &HPX_UTIL_STRIP(component)::HPX_UTIL_STRIP(f)                         \
    /**/

    #define HPX_MAKE_FUNCTION_PTR(...)                                        \
        HPX_UTIL_EXPAND_(BOOST_PP_CAT(                                        \
            HPX_MAKE_FUNCTION_PTR_, HPX_UTIL_PP_NARG(__VA_ARGS__)             \
        )(__VA_ARGS__))                                                       \

    // Macros usable to refer to an action given the function to expose
    #define HPX_MAKE_ACTION(...)                                              \
        ::hpx::actions::action<                                               \
            HPX_UTIL_EXPAND_(HPX_MAKE_FUNCTION_TYPE (__VA_ARGS__))             \
          , HPX_UTIL_EXPAND_(HPX_MAKE_FUNCTION_PTR (__VA_ARGS__))              \
        >                                                                     \
    /**/
    
    #define HPX_MAKE_ACTION_TPL(...)                                          \
        ::hpx::actions::action<                                               \
            HPX_UTIL_EXPAND_(HPX_MAKE_FUNCTION_TYPE_TPL (__VA_ARGS__))         \
          , HPX_UTIL_EXPAND_(HPX_MAKE_FUNCTION_PTR (__VA_ARGS__))              \
        >                                                                     \
    /**/
    
    #define HPX_MAKE_CONST_ACTION(...)                                        \
        ::hpx::actions::action<                                               \
            HPX_UTIL_EXPAND_(HPX_MAKE_CONST_FUNCTION_TYPE (__VA_ARGS__))       \
          , HPX_UTIL_EXPAND_(HPX_MAKE_FUNCTION_PTR (__VA_ARGS__))              \
        >                                                                     \
    /**/
    
    #define HPX_MAKE_CONST_ACTION_TPL(...)                                    \
        ::hpx::actions::action<                                               \
            HPX_UTIL_EXPAND_(HPX_MAKE_CONST_FUNCTION_TYPE_TPL (__VA_ARGS__))   \
          , HPX_UTIL_EXPAND_(HPX_MAKE_FUNCTION_PTR (__VA_ARGS__))              \
        >                                                                     \
    /**/

    /// \endcond
}}

/// \cond NOINTERNAL

#include <hpx/config/warnings_suffix.hpp>

///////////////////////////////////////////////////////////////////////////////
// Helper macro for action serialization, each of the defined actions needs to
// be registered with the serialization library
#define HPX_DEFINE_GET_ACTION_NAME(action)                                    \
    HPX_DEFINE_GET_ACTION_NAME_(action, action)                               \
/**/
#define HPX_DEFINE_GET_ACTION_NAME_(action, actionname)                       \
    namespace hpx { namespace actions { namespace detail {                    \
        template <> HPX_ALWAYS_EXPORT                                         \
        char const* get_action_name<HPX_UTIL_STRIP(action)>()                 \
        {                                                                     \
            return BOOST_PP_STRINGIZE(actionname);                            \
        }                                                                     \
    }}}                                                                       \
/**/

#define HPX_REGISTER_ACTION_(...)                                             \
    HPX_UTIL_EXPAND_(BOOST_PP_CAT(                                            \
        HPX_REGISTER_ACTION_, HPX_UTIL_PP_NARG(__VA_ARGS__)                   \
    )(__VA_ARGS__))                                                           \
/**/
#define HPX_REGISTER_ACTION_1(action)                                         \
    HPX_REGISTER_ACTION_2(action, action)                                     \
/**/

#define HPX_ACTION_EXPORT_IMPLEMENT(action)                                   \
    namespace boost { namespace archive {                                     \
        namespace detail { namespace extra_detail {                           \
            template <>                                                       \
            struct init_guid<HPX_UTIL_STRIP(action)>                          \
            {                                                                 \
                static guid_initializer<HPX_UTIL_STRIP(action)> const & g;    \
            };                                                                \
            guid_initializer<HPX_UTIL_STRIP(action)> const &                  \
                init_guid<HPX_UTIL_STRIP(action)>::g =                        \
                ::boost::serialization::singleton<                            \
                    guid_initializer<HPX_UTIL_STRIP(action)>                  \
                >::get_mutable_instance().export_guid();                      \
    }}}}                                                                      \

#define HPX_REGISTER_ACTION_2(action, actionname)                             \
    HPX_ACTION_EXPORT_IMPLEMENT(                                              \
        (HPX_UTIL_STRIP(action)::, transfer_action_type))                     \
    HPX_REGISTER_BASE_HELPER(                                                 \
        (HPX_UTIL_STRIP(action)::, transfer_action_type), actionname)         \
    HPX_DEFINE_GET_ACTION_NAME_(action, actionname)                           \
/**/

///////////////////////////////////////////////////////////////////////////////
#define HPX_REGISTER_BASE_HELPER(action, actionname)                          \
        hpx::actions::detail::register_base_helper<HPX_UTIL_STRIP(action)>    \
            BOOST_PP_CAT(                                                     \
                BOOST_PP_CAT(__hpx_action_register_base_helper_, __LINE__),   \
                _##actionname);                                               \
    /**/

///////////////////////////////////////////////////////////////////////////////
#define HPX_REGISTER_ACTION_DECLARATION_NO_DEFAULT_GUID1(action)              \
    namespace hpx { namespace actions { namespace detail {                    \
        template <> HPX_ALWAYS_EXPORT                                         \
        char const* get_action_name<HPX_UTIL_STRIP(action)>();                \
    }}}                                                                       \
/**/
#define HPX_REGISTER_ACTION_DECLARATION_NO_DEFAULT_GUID2(action)              \
    namespace hpx { namespace traits {                                        \
        template <>                                                           \
        struct needs_guid_initialization<                                     \
            HPX_UTIL_STRIP(action):: transfer_action_type                     \
        >                                                                     \
          : boost::mpl::false_                                                \
        {};                                                                   \
    }}                                                                        \
/**/
#define HPX_REGISTER_ACTION_DECLARATION_GUID(action)                          \
    namespace boost { namespace archive { namespace detail {                  \
        namespace extra_detail {                                              \
            template <>                                                       \
            struct init_guid<HPX_UTIL_STRIP(action)>;                         \
        }                                                                     \
    }}}                                                                       \
/**/

#define HPX_REGISTER_ACTION_DECLARATION_(...)                                 \
    HPX_UTIL_EXPAND_(BOOST_PP_CAT(                                            \
        HPX_REGISTER_ACTION_DECLARATION_, HPX_UTIL_PP_NARG(__VA_ARGS__)       \
    )(__VA_ARGS__))                                                           \
/**/

#define HPX_CLASS_EXPORT_KEY(action, name)                                    \
    namespace boost { namespace serialization {                               \
        template <>                                                           \
        struct guid_defined<HPX_UTIL_STRIP(action)> : boost::mpl::true_ {};   \
        template <>                                                           \
        inline const char * guid<HPX_UTIL_STRIP(action)>()                    \
        {                                                                     \
            return BOOST_PP_STRINGIZE(name);                                  \
        }                                                                     \
    }}                                                                        \
/**/

#define HPX_REGISTER_ACTION_DECLARATION_1(action)                             \
    HPX_REGISTER_ACTION_DECLARATION_2(action, action)                         \
/**/

#define HPX_REGISTER_ACTION_DECLARATION_2(action, actionname)                 \
    HPX_REGISTER_ACTION_DECLARATION_NO_DEFAULT_GUID1(action)                  \
    HPX_REGISTER_ACTION_DECLARATION_NO_DEFAULT_GUID2(action)                  \
    HPX_CLASS_EXPORT_KEY(                                                     \
        (HPX_UTIL_STRIP(action):: transfer_action_type),                      \
        actionname)                                                           \
    HPX_REGISTER_ACTION_DECLARATION_GUID(                                     \
        (HPX_UTIL_STRIP(action):: transfer_action_type))                      \
/**/


#if 0 //WIP
///////////////////////////////////////////////////////////////////////////////
#define HPX_REGISTER_ACTION_DECLARATION_NO_DEFAULT_GUID1_TEMPLATE(            \
        TEMPLATE, TYPE)                                                       \
    namespace hpx { namespace actions { namespace detail {                    \
        HPX_UTIL_STRIP(TEMPLATE) HPX_ALWAYS_EXPORT                            \
        char const* get_action_name<HPX_UTIL_STRIP(TYPE)>();                  \
    }}}                                                                       \
/**/
#define HPX_REGISTER_ACTION_DECLARATION_NO_DEFAULT_GUID2_TEMPLATE(            \
        TEMPLATE, TYPE)                                                       \
    namespace hpx { namespace traits {                                        \
        HPX_UTIL_STRIP(TEMPLATE)                                              \
        struct needs_guid_initialization<HPX_UTIL_STRIP(TYPE)>                \
          : boost::mpl::false_                                                \
        {};                                                                   \
    }}                                                                        \
/**/
#define HPX_REGISTER_ACTION_DECLARATION_GUID_TEMPLATE(TEMPLATE, TYPE)         \
    namespace boost { namespace archive { namespace detail {                  \
        namespace extra_detail {                                              \
            HPX_UTIL_STRIP(TEMPLATE)                                          \
            struct init_guid<HPX_UTIL_STRIP(TYPE)>;                           \
        }                                                                     \
    }}}                                                                       \
/**/
#define HPX_REGISTER_ACTION_DECLARATION_TEMPLATE(TEMPLATE, TYPE)              \
    HPX_REGISTER_ACTION_DECLARATION_NO_DEFAULT_GUID1_TEMPLATE(                \
        TEMPLATE, HPX_UTIL_STRIP(TYPE))                                       \
    HPX_REGISTER_ACTION_DECLARATION_NO_DEFAULT_GUID2_TEMPLATE(                \
        TEMPLATE, hpx::actions::transfer_action<HPX_UTIL_STRIP(TYPE)>)        \
    HPX_SERIALIZATION_REGISTER_TEMPLATE(                                      \
        TEMPLATE, hpx::actions::transfer_action<HPX_UTIL_STRIP(TYPE)>)        \
    HPX_REGISTER_ACTION_DECLARATION_GUID_TEMPLATE(                            \
        TEMPLATE, hpx::actions::transfer_action<HPX_UTIL_STRIP(TYPE)>)        \
/**/
#endif

///////////////////////////////////////////////////////////////////////////////
#define HPX_ACTION_USES_STACK(action, size)                                   \
    namespace hpx { namespace traits                                          \
    {                                                                         \
        template <typename Enable>                                            \
        struct action_stacksize<action, Enable>                               \
        {                                                                     \
            enum { value = size };                                            \
        };                                                                    \
    }}                                                                        \
/**/

#define HPX_ACTION_USES_SMALL_STACK(action)                                   \
    HPX_ACTION_USES_STACK(action, threads::thread_stacksize_small)            \
/**/
#define HPX_ACTION_USES_MEDIUM_STACK(action)                                  \
    HPX_ACTION_USES_STACK(action, threads::thread_stacksize_medium)           \
/**/
#define HPX_ACTION_USES_LARGE_STACK(action)                                   \
    HPX_ACTION_USES_STACK(action, threads::thread_stacksize_large)            \
/**/
#define HPX_ACTION_USES_HUGE_STACK(action)                                    \
    HPX_ACTION_USES_STACK(action, threads::thread_stacksize_huge)             \
/**/

///////////////////////////////////////////////////////////////////////////////
#define HPX_ACTION_HAS_PRIORITY(action, priority)                             \
    namespace hpx { namespace traits                                          \
    {                                                                         \
        template <typename Enable>                                            \
        struct action_priority<action, Enable>                                \
        {                                                                     \
            enum { value = priority };                                        \
        };                                                                    \
    }}                                                                        \
/**/

#define HPX_ACTION_HAS_LOW_PRIORITY(action)                                   \
    HPX_ACTION_HAS_PRIORITY(action, threads::thread_priority_low)             \
/**/
#define HPX_ACTION_HAS_NORMAL_PRIORITY(action)                                \
    HPX_ACTION_HAS_PRIORITY(action, threads::thread_priority_normal)          \
/**/
#define HPX_ACTION_HAS_CRITICAL_PRIORITY(action)                              \
    HPX_ACTION_HAS_PRIORITY(action, threads::thread_priority_critical)        \
/**/

///////////////////////////////////////////////////////////////////////////////
#define HPX_ACTION_DIRECT_EXECUTION(action)                                   \
    namespace hpx { namespace traits                                          \
    {                                                                         \
        template <typename Enable>                                            \
        struct direct_action<action, Enable>                                  \
            : boost::mpl::true_                                               \
        {};                                                                   \
    }}                                                                        \
/**/

/// \endcond

/// \def HPX_REGISTER_ACTION_DECLARATION(action)
///
/// \brief Declare the necessary component action boilerplate code.
///
/// The macro \a HPX_REGISTER_ACTION_DECLARATION can be used to declare all the
/// boilerplate code which is required for proper functioning of component
/// actions in the context of HPX.
///
/// The parameter \a action is the type of the action to declare the
/// boilerplate for.
/// 
/// This macro can be invoked with an optional second parameter. This parameter
/// specifies a unique name of the action to be used for serialization purposes. 
/// The second parameter has to be specified if the first parameter is not 
/// usable as a plain (non-qualified) C++ identifier, i.e. the first parameter
/// contains special characters which cannot be part of a C++ identifier, such 
/// as '<', '>', or ':'. 
///
/// \par Example:
///
/// \code
///      namespace app
///      {
///          // Define a simple component exposing one action 'print_greating'
///          class HPX_COMPONENT_EXPORT server
///            : public hpx::components::simple_component_base<server>
///          {
///              void print_greating ()
///              {
///                  hpx::cout << "Hey, how are you?\n" << hpx::flush;
///              }
///
///              // Component actions need to be declared, this also defines the
///              // type 'print_greating_action' representing the action.
///              HPX_DEFINE_COMPONENT_ACTION(server, print_greating, print_greating_action);
///          };
///      }
///
///      // Declare boilerplate code required for each of the component actions.
///      HPX_REGISTER_ACTION_DECLARATION(app::server::print_greating_action);
/// \endcode
///
/// \note This macro has to be used once for each of the component actions
/// defined using one of the \a HPX_DEFINE_COMPONENT_ACTION macros. It has to
/// be visible in all translation units using the action, thus it is
/// recommended to place it into the header file defining the component.
#define HPX_REGISTER_ACTION_DECLARATION(...)                                  \
    HPX_REGISTER_ACTION_DECLARATION_(__VA_ARGS__)                             \
/**/

/// \def HPX_REGISTER_ACTION_DECLARATION_TEMPLATE(template, action)
///
/// \brief Declare the necessary component action boilerplate code for actions
///        taking template type arguments.
///
/// The macro \a HPX_REGISTER_ACTION_DECLARATION_TEMPLATE can be used to
/// declare all the boilerplate code which is required for proper functioning
/// of component actions in the context of HPX, if those actions take template
/// type arguments.
///
/// The parameter \a template specifies the list of template type declarations
/// for the action type. This argument has to be wrapped into an additional
/// pair of parenthesis.
///
/// The parameter \a action is the type of the action to declare the
/// boilerplate for. This argument has to be wrapped into an additional pair
/// of parenthesis.
///
/// \par Example:
///
/// \code
///      namespace app
///      {
///          // Define a simple component exposing one action 'print_greating'
///          class HPX_COMPONENT_EXPORT server
///            : public hpx::components::simple_component_base<server>
///          {
///              template <typename T>
///              void print_greating (T t)
///              {
///                  hpx::cout << "Hey " << t << ", how are you?\n" << hpx::flush;
///              }
///
///              // Component actions need to be declared, this also defines the
///              // type 'print_greating_action' representing the action.
///
///              // Actions with template arguments (like print_greating<>()
///              // above) require special type definitions. The simplest way
///              // to define such an action type is by deriving from the HPX
///              // facility make_action:
///              template <typename T>
///              struct print_greating_action
///                : hpx::actions::make_action<
///                      void (server::*)(T), &server::template print_greating<T>,
///                      print_greating_action<T> >
///              {};
///          };
///      }
///
///      // Declare boilerplate code required for each of the component actions.
///      HPX_REGISTER_ACTION_DECLARATION_TEMPLATE((template T), (app::server::print_greating_action<T>));
/// \endcode
///
/// \note This macro has to be used once for each of the component actions
/// defined as above. It has to be visible in all translation units using the
/// action, thus it is recommended to place it into the header file defining the
/// component.
#define HPX_REGISTER_ACTION_DECLARATION_TEMPLATE(TEMPLATE, TYPE)              \
    HPX_SERIALIZATION_REGISTER_TEMPLATE_ACTION(TEMPLATE, TYPE)                \
/**/

/// \def HPX_REGISTER_ACTION(action)
///
/// \brief Define the necessary component action boilerplate code.
///
/// The macro \a HPX_REGISTER_ACTION can be used to define all the
/// boilerplate code which is required for proper functioning of component
/// actions in the context of HPX.
///
/// The parameter \a action is the type of the action to define the
/// boilerplate for.
/// 
/// This macro can be invoked with an optional second parameter. This parameter
/// specifies a unique name of the action to be used for serialization purposes. 
/// The second parameter has to be specified if the first parameter is not 
/// usable as a plain (non-qualified) C++ identifier, i.e. the first parameter
/// contains special characters which cannot be part of a C++ identifier, such 
/// as '<', '>', or ':'. 
///
/// \note This macro has to be used once for each of the component actions
/// defined using one of the \a HPX_DEFINE_COMPONENT_ACTION macros. It has to
/// occur exactly once for each of the actions, thus it is recommended to
/// place it into the source file defining the component. There is no need
/// to use this macro for actions which have template type arguments (see
/// \a HPX_REGISTER_ACTION_DECLARATION_TEMPLATE)
#define HPX_REGISTER_ACTION(...)                                              \
    HPX_REGISTER_ACTION_(__VA_ARGS__)                                         \
/**/

#endif

