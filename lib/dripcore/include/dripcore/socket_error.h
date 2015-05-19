// ----------------------------------------------------------------------------
//
//     Filename   : socket_error.h
//
//     Author     : Benny Bach <benny.bach@gmail.com>
//                  Copyright (C) 2015
//
// --- Description: -----------------------------------------------------------
//
//
// ----------------------------------------------------------------------------
#ifndef __socket_error_h__
#define __socket_error_h__

// ----------------------------------------------------------------------------
#include <stdexcept>
#include <system_error>

// ----------------------------------------------------------------------------
class socket_error : public std::system_error
{
public:
    enum
    {
        wouldblock = EWOULDBLOCK,
        again      = EAGAIN
    };
public:
    socket_error(int ec) : std::system_error(ec, std::system_category()) {}
public:
    virtual ~socket_error() noexcept {}
public:
    //bool success() { return code().value() == false; }
    //virtual const char* what() const noexcept;
private:
    //mutable std::string m_msg;
    //int                 m_errno;
};

// ----------------------------------------------------------------------------
#endif // __socket_error_h_
