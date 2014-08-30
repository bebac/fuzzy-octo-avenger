// ----------------------------------------------------------------------------
//
//     Filename   : json_value.cpp
//
//     Author     : Benny Bach <benny.bach@gmail.com>
//                  Copyright (C) 2014
//
// --- Description: -----------------------------------------------------------
//
//
// ----------------------------------------------------------------------------
#include <json/json_value.h>
#include <json/json_array.h>
#include <json/json_object.h>

// ----------------------------------------------------------------------------
namespace json
{
  value::value(const value& other) : type_(type::nul)
  {
    *this = other;
  }

  value::value(value&& other) : type_(type::nul)
  {
    *this = std::move(other);
  }

  value& value::operator= (const value& rhs)
  {
    free_value();
    switch( rhs.type_id() )
    {
      case json::type::nul:
        break;
      case json::type::str:
        str_ = new std::string(*rhs.str_);
        break;
      case json::type::num:
        num_ = rhs.num_;
        break;
      case json::type::tru:
      case json::type::fal:
        t_f_ = rhs.t_f_;
        break;
      case json::type::arr:
        arr_ = new json::array(*rhs.arr_);
        break;
      case json::type::obj:
        obj_ = new json::object(*rhs.obj_);
        break;
      default:
        assert(false);
        break;
    }
    type_ = rhs.type_;
    return *this;
  }

  value& value::operator= (value&& rhs)
  {
    free_value();
    switch( rhs.type_id() )
    {
      case json::type::nul:
        break;
      case json::type::str:
        str_ = rhs.str_; rhs.str_ = 0;
        break;
      case json::type::num:
        num_ = rhs.num_;
        break;
      case json::type::tru:
      case json::type::fal:
        t_f_ = rhs.t_f_;
        break;
      case json::type::arr:
        arr_ = rhs.arr_; rhs.arr_ = 0;
        break;
      case json::type::obj:
        obj_ = rhs.obj_; rhs.obj_ = 0;
        break;
      default:
        assert(false);
        break;
    }
    type_ = rhs.type_;
    rhs.type_ = json::type::nul;
    return *this;
  }

  value::value(const char* v)
    :
    type_(type::str), str_(new std::string(v))
  {
  }

  value::value(const std::string& v)
    :
    type_(type::str), str_(new std::string(v))
  {
  }

  value::value(std::string&& v)
    :
    type_(type::str), str_(new std::string(std::move(v)))
  {
  }

  value::value(double v)
    :
    type_(type::num), num_(v)
  {
  }

  value::value(int v)
    :
    type_(type::num), num_(v)
  {
  }

  value::value(unsigned v)
    :
    type_(type::num), num_(v)
  {
  }

  value::value(bool v)
    :
    type_(v ? type::tru : type::fal), t_f_(v)
  {
  }

  value::value(const json::array& v)
    :
    type_(type::arr), arr_(new array(v))
  {
  }

  value::value(json::array&& v)
    :
    type_(type::arr), arr_(new array(std::move(v)))
  {
  }

  value::value(const json::object& v)
    :
    type_(type::obj), obj_(new object(v))
  {
  }

  value::value(json::object&& v)
    :
    type_(type::obj), obj_(new object(std::move(v)))
  {
  }

  value::~value()
  {
    free_value();
  }

  void value::write(std::ostream& os) const
  {
    switch ( type_ )
    {
      case json::type::nul:
        os << "null";
        break;
      case json::type::str:
        os << escape(*str_);
        break;
      case json::type::num:
        os << num_;
        break;
      case json::type::tru:
        os << "true";
        break;
      case json::type::fal:
        os << "false";
        break;
      case json::type::arr:
        os << *arr_;
        break;
      case json::type::obj:
        os << *obj_;
        break;
      default:
        assert(false);
        break;
    }
  }

  void value::free_value()
  {
    if ( type_ >= type::str )
    {
      switch( type_ )
      {
        case json::type::str:
          delete str_; str_ = 0;
          break;
        case json::type::arr:
          delete arr_; arr_ = 0;
          break;
        case json::type::obj:
          delete obj_; obj_ = 0;
          break;
        default:
          assert(false);
          break;
      }
    }
  }
}

// ----------------------------------------------------------------------------
std::ostream& operator<<(std::ostream& os, json::type t)
{
  switch ( t )
  {
    case json::type::nul: os << "null";   break;
    case json::type::str: os << "string"; break;
    case json::type::num: os << "number"; break;
    case json::type::tru: os << "true";   break;
    case json::type::fal: os << "false";  break;
    case json::type::arr: os << "array";  break;
    case json::type::obj: os << "object"; break;
    default:
      os << "<unknown>";
      break;
  }
  return os;
}
