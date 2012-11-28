//  Copyright (c) 2007-2012 Hartmut Kaiser
//  Copyright (c)      2011 Bryce Lelbach
//  Copyright (c)      2011 Thomas Heller
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file transfer_action.hpp

#if !defined(HPX_RUNTIME_ACTIONS_TRANSFER_ACTION_HPP)
#define HPX_RUNTIME_ACTIONS_TRANSFER_ACTION_HPP

#include <hpx/runtime/actions/action_support.hpp>

namespace hpx { namespace actions {
    ///////////////////////////////////////////////////////////////////////////
    template <typename F, F funcptr>
    struct transfer_action : base_action
    {
        typedef action<F, funcptr> action_type;
        typedef typename action_type::impl action_impl_type;
        typedef typename action_impl_type::component_type component_type;
        typedef typename action_impl_type::result_type result_type;
        typedef typename action_impl_type::arguments_type arguments_type;

        // This is the priority value this action has been instantiated with
        // (statically). This value might be different from the priority member
        // holding the runtime value an action has been created with
        enum { priority_value = traits::action_priority<action_type>::value };

        // This is the stacksize value this action has been instantiated with
        // (statically). This value might be different from the stacksize member
        // holding the runtime value an action has been created with
        enum { stacksize_value = traits::action_stacksize<action_type>::value };

        // default constructor is needed for serialization
        transfer_action() {}

        // construct an action from its arguments
        explicit transfer_action(threads::thread_priority priority)
          : arguments_(),
#if HPX_THREAD_MAINTAIN_PARENT_REFERENCE
            parent_locality_(transfer_action::get_locality_id()),
            parent_id_(reinterpret_cast<boost::uint64_t>(threads::get_parent_id())),
            parent_phase_(threads::get_parent_phase()),
#endif
            priority_(
                detail::thread_priority<
                    static_cast<threads::thread_priority>(priority_value)
                >::call(priority)),
            stacksize_(
                detail::thread_stacksize<
                    static_cast<threads::thread_stacksize>(stacksize_value)
                >::call(threads::thread_stacksize_default))
        {}

        template <typename Arg0>
        explicit transfer_action(BOOST_FWD_REF(Arg0) arg0)
          : arguments_(boost::forward<Arg0>(arg0)),
#if HPX_THREAD_MAINTAIN_PARENT_REFERENCE
            parent_locality_(transfer_action::get_locality_id()),
            parent_id_(reinterpret_cast<boost::uint64_t>(threads::get_parent_id())),
            parent_phase_(threads::get_parent_phase()),
#endif
            priority_(
                detail::thread_priority<
                    static_cast<threads::thread_priority>(priority_value)
                >::call(threads::thread_priority_default)),
            stacksize_(
                detail::thread_stacksize<
                    static_cast<threads::thread_stacksize>(stacksize_value)
                >::call(threads::thread_stacksize_default))
        {}

        template <typename Arg0>
        transfer_action(threads::thread_priority priority, BOOST_FWD_REF(Arg0) arg0)
          : arguments_(boost::forward<Arg0>(arg0)),
#if HPX_THREAD_MAINTAIN_PARENT_REFERENCE
            parent_locality_(transfer_action::get_locality_id()),
            parent_id_(reinterpret_cast<boost::uint64_t>(threads::get_parent_id())),
            parent_phase_(threads::get_parent_phase()),
#endif
            priority_(
                detail::thread_priority<
                    static_cast<threads::thread_priority>(priority_value)
                >::call(priority)),
            stacksize_(
                detail::thread_stacksize<
                    static_cast<threads::thread_stacksize>(stacksize_value)
                >::call(threads::thread_stacksize_default))
        {}

        // bring in the rest of the constructors
#if HPX_THREAD_MAINTAIN_PARENT_REFERENCE
        #include <hpx/runtime/actions/transfer_action_constructors.hpp>
#else
        #include <hpx/runtime/actions/transfer_action_constructors_no_parent_reference.hpp>
#endif

        //
        ~transfer_action()
        {
            detail::guid_initialization<transfer_action>();
        }

    public:
        /// retrieve component type
        static int get_static_component_type()
        {
            return
                static_cast<int>(
                    components::get_component_type<
                        component_type
                    >()
                );
        }

    private:
        /// The function \a get_component_type returns the \a component_type
        /// of the component this action belongs to.
        int get_component_type() const
        {
            return
                static_cast<int>(
                    components::get_component_type<
                        component_type
                    >()
                );
        }

        /// The function \a get_action_name returns the name of this action
        /// (mainly used for debugging and logging purposes).
        char const* get_action_name() const
        {
            return detail::get_action_name<action_type>();
        }

        /// The function \a get_action_type returns whether this action needs
        /// to be executed in a new thread or directly.
        base_action::action_type get_action_type() const
        {
            return detail::get_action_type<action_type>::call();
        }

        /// The \a get_thread_function constructs a proper thread function for
        /// a \a thread, encapsulating the functionality and the arguments
        /// of the action it is called for.
        ///
        /// \param lva    [in] This is the local virtual address of the
        ///               component the action has to be invoked on.
        ///
        /// \returns      This function returns a proper thread function usable
        ///               for a \a thread.
        ///
        /// \note This \a get_thread_function will be invoked to retrieve the
        ///       thread function for an action which has to be invoked without
        ///       continuations.
        HPX_STD_FUNCTION<threads::thread_function_type>
        get_thread_function(naming::address::address_type lva)
        {
            return boost::move(action_impl_type::construct_thread_function(
                lva, arguments_));
        }

        /// The \a get_thread_function constructs a proper thread function for
        /// a \a thread, encapsulating the functionality, the arguments, and
        /// the continuations of the action it is called for.
        ///
        /// \param cont   [in] This is the list of continuations to be
        ///               triggered after the execution of the action
        /// \param lva    [in] This is the local virtual address of the
        ///               component the action has to be invoked on.
        ///
        /// \returns      This function returns a proper thread function usable
        ///               for a \a thread.
        ///
        /// \note This \a get_thread_function will be invoked to retrieve the
        ///       thread function for an action which has to be invoked with
        ///       continuations.
        HPX_STD_FUNCTION<threads::thread_function_type>
        get_thread_function(continuation_type& cont,
            naming::address::address_type lva)
        {
            return boost::move(action_impl_type::construct_thread_function(
                cont, lva, arguments_));
        }

#if !HPX_THREAD_MAINTAIN_PARENT_REFERENCE
        /// Return the locality of the parent thread
        boost::uint32_t get_parent_locality_id() const
        {
            return naming::invalid_locality_id;
        }

        /// Return the thread id of the parent thread
        threads::thread_id_type get_parent_thread_id() const
        {
            return threads::invalid_thread_id;
        }

        /// Return the phase of the parent thread
        boost::uint64_t get_parent_thread_phase() const
        {
            return 0;
        }
#else
        /// Return the locality of the parent thread
        boost::uint32_t get_parent_locality_id() const
        {
            return parent_locality_;
        }

        /// Return the thread id of the parent thread
        threads::thread_id_type get_parent_thread_id() const
        {
            return reinterpret_cast<threads::thread_id_type>(parent_id_);
        }

        /// Return the phase of the parent thread
        boost::uint64_t get_parent_thread_phase() const
        {
            return parent_phase_;
        }
#endif

        /// Return the thread priority this action has to be executed with
        threads::thread_priority get_thread_priority() const
        {
            return priority_;
        }

        /// Return the thread stacksize this action has to be executed with
        threads::thread_stacksize get_thread_stacksize() const
        {
            return stacksize_;
        }

        /// Return the size of action arguments in bytes
        std::size_t get_type_size() const
        {
            return traits::type_size<arguments_type>::call(arguments_);
        }

        /// Return all data needed for thread initialization
        threads::thread_init_data&
        get_thread_init_data(naming::address::address_type lva,
            threads::thread_init_data& data)
        {
            data.func = boost::move(action_impl_type::construct_thread_function(lva, arguments_));
#if HPX_THREAD_MAINTAIN_TARGET_ADDRESS
            data.lva = lva;
#endif
#if HPX_THREAD_MAINTAIN_DESCRIPTION
            data.description = detail::get_action_name<action_type>();
#endif
#if HPX_THREAD_MAINTAIN_PARENT_REFERENCE
            data.parent_id = reinterpret_cast<threads::thread_id_type>(parent_id_);
            data.parent_locality_id = parent_locality_;
#endif
            data.priority = priority_;
            data.stacksize = threads::get_stack_size(stacksize_);
            return data;
        }

        threads::thread_init_data&
        get_thread_init_data(continuation_type& cont,
            naming::address::address_type lva, threads::thread_init_data& data)
        {
            data.func = boost::move(action_impl_type::construct_thread_function(cont, lva, arguments_));
#if HPX_THREAD_MAINTAIN_TARGET_ADDRESS
            data.lva = lva;
#endif
#if HPX_THREAD_MAINTAIN_DESCRIPTION
            data.description = detail::get_action_name<action_type>();
#endif
#if HPX_THREAD_MAINTAIN_PARENT_REFERENCE
            data.parent_id = reinterpret_cast<threads::thread_id_type>(parent_id_);
            data.parent_locality_id = parent_locality_;
#endif
            data.priority = priority_;
            data.stacksize = threads::get_stack_size(stacksize_);
            return data;
        }

    public:
        /// retrieve the N's argument
        template <int N>
        typename boost::fusion::result_of::at_c<arguments_type, N>::type
        get()
        {
            return boost::fusion::at_c<N>(arguments_);
        }

        /// serialization support
        static void register_base()
        {
            util::void_cast_register_nonvirt<transfer_action, base_action>();
        }

    private:
        // serialization support
        friend class boost::serialization::access;

        template <class Archive>
        void serialize(Archive& ar, const unsigned int /*version*/)
        {
            util::serialize_sequence(ar, arguments_);

            // Always serialize the parent information to maintain binary 
            // compatibility on the wire.
#if !HPX_THREAD_MAINTAIN_PARENT_REFERENCE
            boost::uint32_t parent_locality_ = naming::invalid_locality_id;
            boost::uint64_t parent_id_ = boost::uint64_t(-1);
            boost::uint64_t parent_phase_ = 0;
#endif
            ar & parent_locality_;
            ar & parent_id_;
            ar & parent_phase_;

            ar & priority_;
            ar & stacksize_;
        }

    private:
        static boost::uint32_t get_locality_id()
        {
            error_code ec(lightweight);      // ignore any errors
            return hpx::get_locality_id(ec);
        }

    protected:
        arguments_type arguments_;
#if HPX_THREAD_MAINTAIN_PARENT_REFERENCE
        boost::uint32_t parent_locality_;
        boost::uint64_t parent_id_;
        boost::uint64_t parent_phase_;
#endif
        threads::thread_priority priority_;
        threads::thread_stacksize stacksize_;
    };

    ///////////////////////////////////////////////////////////////////////////
    template <int N, typename F, F funcptr>
    inline typename boost::fusion::result_of::at_c<
        typename transfer_action<F, funcptr>::arguments_type, N
    >::type
    get(transfer_action<F, funcptr> & args)
    {
        return args.get<N>();
    }
}}

#endif
