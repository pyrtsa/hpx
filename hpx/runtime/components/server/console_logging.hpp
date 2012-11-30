//  Copyright (c) 2007-2012 Hartmut Kaiser
//  Copyright (c)      2011 Bryce Lelbach
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(HPX_COMPONENTS_CONSOLE_DEC_16_2008_0427PM)
#define HPX_COMPONENTS_CONSOLE_DEC_16_2008_0427PM

#include <string>
#include <vector>

#include <hpx/hpx_fwd.hpp>
#include <hpx/runtime/components/component_type.hpp>
#include <hpx/runtime/actions/plain_action.hpp>
#include <hpx/util/logging.hpp>
#include <hpx/util/void_cast.hpp>

#include <boost/fusion/include/vector.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace hpx { namespace components
{
    typedef boost::fusion::vector3<
        logging_destination, std::size_t, std::string
    > message_type;

    typedef std::vector<message_type> messages_type;
}}

///////////////////////////////////////////////////////////////////////////////
// non-intrusive serialization
namespace boost { namespace serialization
{
    template <typename Archive>
    void serialize(Archive&, hpx::components::message_type&, unsigned int const);
}}

//////////////////////////////////////////////////////////////////////////////
namespace hpx { namespace components { namespace server
{
    ///////////////////////////////////////////////////////////////////////////
    // console logging happens here
    void console_logging(messages_type const&);
    HPX_DEFINE_PLAIN_ACTION(console_logging, console_logging_action);

    ///////////////////////////////////////////////////////////////////////////
    // this type is a dummy template to avoid premature instantiation of the
    // serialization support instances
#if 0
    template <typename Dummy = void>
    class console_logging_action
      : public
            HPX_MAKE_ACTION_DERIVED(
                console_logging, console_logging_action<Dummy>
            )::type
    {
    private:
        typedef typename
            HPX_MAKE_ACTION_DERIVED(
                console_logging, console_logging_action
            )::type
        base_type;

    public:
        console_logging_action() {}

        // construct an action from its arguments
        console_logging_action(messages_type const& msgs)
          : base_type(msgs)
        {}

        console_logging_action(threads::thread_priority,
                messages_type const& msgs)
          : base_type(msgs)
        {}

        /// serialization support
        static void register_base()
        {
            util::void_cast_register_nonvirt<console_logging_action, base_type>();
            base_type::register_base();
        }

    public:

    private:
        // serialization support
        friend class boost::serialization::access;

        template<class Archive>
        void serialize(Archive& ar, const unsigned int /*version*/)
        {
            ar & boost::serialization::base_object<base_type>(*this);
        }
    };
#endif
}}}

HPX_ACTION_DIRECT_EXECUTION(
    hpx::components::server::console_logging_action
)

namespace hpx { namespace actions {
    template <>
    struct action_impl<
        HPX_MAKE_FUNCTION_TYPE(hpx::components::server::console_logging),
        HPX_MAKE_FUNCTION_PTR(hpx::components::server::console_logging)
    >
    {
        typedef void (*funcptr_type)(hpx::components::messages_type const &);
        typedef hpx::util::unused_type result_type;
        typedef
            components::server::plain_function<hpx::components::server::console_logging_action>
            component_type;
        typedef hpx::util::tuple1<hpx::components::messages_type> arguments_type;

        template <typename Arguments>
        static util::unused_type
        execute_function(naming::address::address_type lva,
            BOOST_FWD_REF(Arguments) args)
        {
            try {
                // call the function, ignoring the return value
                hpx::components::server::console_logging(
                    boost::move(boost::fusion::at_c<0>(args)));
            }
            catch (hpx::exception const& /*e*/) {
                /**/;      // no logging!
            }
            return util::unused;
        }
        
        template <typename Arguments>
        static HPX_STD_FUNCTION<threads::thread_function_type>
        construct_thread_function(naming::address::address_type lva,
            BOOST_FWD_REF(Arguments) /*args*/)
        {
            BOOST_ASSERT(false);
            return HPX_STD_FUNCTION<threads::thread_function_type>();
        }
        
        template <typename Arguments>
        static HPX_STD_FUNCTION<threads::thread_function_type>
        construct_thread_function(continuation_type& cont,
            naming::address::address_type lva, BOOST_FWD_REF(Arguments) args)
        {
            BOOST_ASSERT(false);
            return HPX_STD_FUNCTION<threads::thread_function_type>();
        }
    };
}}

HPX_REGISTER_PLAIN_ACTION_DECLARATION(
    hpx::components::server::console_logging_action
)

namespace hpx { namespace traits
{
    template <>
    struct needs_guid_initialization<hpx::components::server::console_logging_action::transfer_action_type >
      : boost::mpl::false_
    {};
}}

#endif

