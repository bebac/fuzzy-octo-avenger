// ----------------------------------------------------------------------------
//
//     Filename   : json_object.h
//
//     Author     : Benny Bach <benny.bach@gmail.com>
//                  Copyright (C) 2014
//
// --- Description: -----------------------------------------------------------
//
//
// ----------------------------------------------------------------------------
#ifndef __json__object_h__
#define __json__object_h__

// ----------------------------------------------------------------------------
#include <json/json_value.h>

// ----------------------------------------------------------------------------
#include <unordered_map>

// ----------------------------------------------------------------------------
namespace json
{
  class value;

  class object
  {
  public:
    using member_map = std::unordered_map<std::string, value>;
  public:
    object() : value_() {}
    object(std::initializer_list<member_map::value_type> v) : value_(v) {}
  public:
    object(const object& other)
    {
      *this = other;
    }
  public:
    object(object&& other)
    {
      *this = std::move(other);
    }
  public:
    object& operator= (const object& rhs)
    {
      value_ = rhs.value_;
      return *this;
    }
  public:
    object& operator= (object&& rhs)
    {
      value_ = std::move(rhs.value_);
      return *this;
    }
  public:
    virtual ~object() {}
  public:
    bool empty() { return value_.empty(); }
  public:
    virtual bool is_object() const { return true; }
  public:
    bool has_member(std::string key) const
    {
      return value_.find(key) != end(value_);
    }
  public:
    virtual void write(std::ostream& os) const;
  public:
    template <typename K> value& operator[](K key) { return value_[std::forward<K>(key)]; }
    template <typename K> const value& at(K key) const { return value_.at(std::forward<K>(key)); }
  public:
    template <typename K, typename V> void member(K key, V v)
    {
      value_.emplace(std::forward<K>(key), std::forward<V>(v));
    }
  private:
    member_map value_;
  };

  inline void object::write(std::ostream& os) const
  {
    os << "{";
    for ( member_map::const_iterator it = begin(value_); it != end(value_); )
    {
      os << escape((*it).first) << ":" << (*it).second;

      if ( ++it != end(value_) ) {
        os << ",";
      }
    }
    os << "}";
  }
}

// ----------------------------------------------------------------------------
inline std::ostream& operator<<(std::ostream& os, const json::object& v)
{
  v.write(os);
  return os;
}

// ----------------------------------------------------------------------------
inline std::string to_string(const json::object& value)
{
  std::stringstream os;
  os << value;
  return os.str();
}

// ----------------------------------------------------------------------------
#endif // __json__object_h__
