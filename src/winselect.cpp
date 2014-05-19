#include "winselect.hpp"
#include "err.hpp"
#include <map>
#include "signaler.hpp"

// From the CE5 sources, wsock.h
#define FD_FAILED_CONNECT   0x0100

int winselect (
        int nfds,
        fd_set* readfds,
        fd_set* writefds,
        fd_set* exceptfds,
        const struct timeval FAR * timeout
    )
{
#ifdef ZMQ_HAVE_WINCE
    WSAEVENT eventsToWaitFor[1] = {0};
    size_t eventCount = 1;
    zmq::signaler_t* signalers[FD_SETSIZE];
    size_t signalerCount = 0;

    // SOCKETS EVENT
    eventsToWaitFor[0] = WSACreateEvent();

    std::map<SOCKET, long> sockEvents;

    size_t i;
    if (readfds) {
        for (i=0; i < readfds->fd_count; ++i) {
            // Assume that the entry is a socket. Try associating it to the event
            SOCKET sock = readfds->fd_array[i];
            sockEvents[sock] |= FD_READ | FD_CLOSE | FD_ACCEPT;
        }
    }

    if (writefds) {
        for (i=0; i < writefds->fd_count; ++i) {
            // Assume that the entry is a socket. Try associating it to the event
            SOCKET sock = writefds->fd_array[i];
            sockEvents[sock] |= FD_WRITE | FD_CONNECT;
        }
    }

    if (exceptfds) {
        for (i=0; i < exceptfds->fd_count; ++i) {
            // Assume that the entry is a socket. Try associating it to the event
            SOCKET sock = exceptfds->fd_array[i];
            sockEvents[sock] |= FD_OOB | FD_FAILED_CONNECT;
        }
    }

    std::map<SOCKET, long>::iterator it;
    for (it = sockEvents.begin(); it != sockEvents.end(); ++it) {
        int rc = WSAEventSelect(it->first, eventsToWaitFor[0], it->second);
        if (rc == SOCKET_ERROR) {
            DWORD err = WSAGetLastError();
            if (err == WSAENOTSOCK) {
                // This is not a socket! Assume it is a signaler, so
                // add ourselves to the list of people who'd like to get
                // a heads-up when it wakes up
                zmq::signaler_t* signaler = (zmq::signaler_t*) it->first;
                signaler->addWaitingEvent((zmq::fd_t) eventsToWaitFor[0]);
                signalers[signalerCount++] = signaler;
                zmq_assert(signalerCount <= FD_SETSIZE);
            } else {
                // Some other type of error that should definitely not happen.
                wsa_assert_no(err);
            }
        }
    }

    DWORD timeoutMs = WSA_INFINITE;
    if (timeout) {
        timeoutMs = (timeout->tv_sec*1000) + (timeout->tv_usec/1000);
    }

    //printf("wait %x/%x/%d\n", eventsToWaitFor[0], eventsToWaitFor[1], timeoutMs);

    // Wait for any of the events...
    DWORD ret = WSAWaitForMultipleEvents(eventCount,
            eventsToWaitFor, FALSE, timeoutMs, FALSE);
    DWORD err = WSAGetLastError();

    // Deregister ourselves from the signalers
    for (i=0; i < signalerCount; ++i) {
        signalers[i]->removeWaitingEvent((zmq::fd_t) eventsToWaitFor[0]);
    }

    WSACloseEvent(eventsToWaitFor[0]);


    if (ret >= WSA_WAIT_EVENT_0 && ret < WSA_WAIT_EVENT_0 + eventCount) {
        // OK!
        WSASetLastError(err);
        return 1;
    } else if (ret == WSA_WAIT_TIMEOUT) {
        // Timeout.
        WSASetLastError(err);
        return 0;
    } else {
        // Error.
        WSASetLastError(err);
        return SOCKET_ERROR;
    }
#else
    return select(nfds, readfds, writefds, exceptfds, timeout);
#endif
}
