// ----------------------------------------------------------------------------
//
//     Filename   : dripcore/connection.cpp
//
//     Author     : Benny Bach <benny.bach@gmail.com>
//                  Copyright (C) 2015
//
// --- Description: -----------------------------------------------------------
//
//
// ----------------------------------------------------------------------------
#include <dripcore/connection.h>

// ----------------------------------------------------------------------------
#include <iostream>
#include <sstream>
#include <cstring>

// ----------------------------------------------------------------------------
namespace dripcore
{
  connection::connection(dripcore::socket socket)
    :
    eventable(),
    context_(),
    socket_(std::move(socket))
  {
    set_rd_handler(std::bind(&connection::read, this));
    set_wr_handler(std::bind(&connection::write, this));
  }

  connection::~connection()
  {
    socket_.close();
  }

  void connection::read()
  {
    std::array<char, 1024> buf;

    ssize_t res;
    do
    {
      res = socket_.recv(buf.data(), buf.size(), 0);

      if ( res > 0 )
      {
        receive_data(buf.data(), res);
      }
      else if ( res < 0 )
      {
        if ( socket_.last_error() == ECONNRESET )
        {
          stop();
        }
      }
      else
      {
        stop();
      }
    }
    while ( res != 0 && res == buf.size() );
  }

  void connection::send(const char* buf, size_t len)
  {
    // If there is currently no data in the output buffer try to send
    // the data directly onto the socket. If the send would block write
    // it to the output buffer and wait for the next write event.

    if ( obuf_len() == 0 )
    {
      ssize_t sent = socket_.send(buf, len, 0);

      if ( sent >= 0 )
      {
        if ( static_cast<size_t>(sent) < len ) {
          send(buf+sent, len-sent);
        }
      }
      else if ( errno == EWOULDBLOCK || errno == EAGAIN )
      {
        obuf_write(buf, len);
      }
      else
      {
        std::cerr << "connection send error " << strerror(errno) << std::endl;
        stop();
      }
    }
    else
    {
      obuf_write(buf, len);
    }
  }

  void connection::write()
  {
    if ( obuf_len() == 0 ) {
      return;
    }

    ssize_t sent = socket_.send(obuf_ptr(), obuf_len(), 0);

    if ( sent >= 0 )
    {
      obuf_sent(sent);
    }
    else if ( errno == EWOULDBLOCK || errno == EAGAIN )
    {
    }
    else
    {
      std::cerr << "connection write error " << strerror(errno) << std::endl;
      stop();
    }
  }
}