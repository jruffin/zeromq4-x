/*
    Copyright (c) 2007-2010 iMatix Corporation

    This file is part of 0MQ.

    0MQ is free software; you can redistribute it and/or modify it under
    the terms of the Lesser GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    0MQ is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    Lesser GNU General Public License for more details.

    You should have received a copy of the Lesser GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __ZMQ_PUSH_HPP_INCLUDED__
#define __ZMQ_PUSH_HPP_INCLUDED__

#include "i_terminate_events.hpp"
#include "socket_base.hpp"
#include "lb.hpp"

namespace zmq
{

    class push_t : public socket_base_t, public i_terminate_events
    {
    public:

        push_t (class ctx_t *parent_, uint32_t slot_);
        ~push_t ();

    protected:

        //  Overloads of functions from socket_base_t.
        void xattach_pipes (class reader_t *inpipe_, class writer_t *outpipe_,
            const blob_t &peer_identity_);
        int xsend (zmq_msg_t *msg_, int flags_);
        bool xhas_out ();

    private:

        //  i_terminate_events interface implementation.
        void terminated ();

        //  Hook into the termination process.
        void process_term ();

        //  Load balancer managing the outbound pipes.
        lb_t lb;

        push_t (const push_t&);
        void operator = (const push_t&);
    };

}

#endif