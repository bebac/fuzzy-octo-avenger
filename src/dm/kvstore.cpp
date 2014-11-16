// ----------------------------------------------------------------------------
//
//     Filename   : kvstore.cpp
//
//     Author     : Benny Bach <benny.bach@gmail.com>
//                  Copyright (C) 2014
//
// --- Description: -----------------------------------------------------------
//
//
// ----------------------------------------------------------------------------
#include "kvstore.h"

// ----------------------------------------------------------------------------
#include <stdexcept>

// ----------------------------------------------------------------------------
namespace dm
{

  char alphabet[] = "0123456789"
                    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                    "abcdefghijklmnopqrstuvwxyz";

  auto base       = sizeof(alphabet)-1;

  std::string base62_encode(unsigned long long value)
  {
      std::string result;

      do
      {
        result.push_back(alphabet[value % base]);
        value /= base;
      }
      while(value > 0);

      std::reverse(result.begin(), result.end());

      return std::move(result);
  }

  unsigned long long base62_decode(const std::string& value)
  {
    unsigned long long result = 0;

    auto len = value.length();

    for ( unsigned i=len; i>0; i-- )
    {
      int c = value[i-1];
      int v;

      if ( c >= '0' && c <= '9') {
        v = c - '0';
      }
      else if ( c >= 'A' && c <= 'Z' ) {
        v = c - 'A' + 10;
      }
      else if ( c >= 'a' && c <= 'z' ) {
        v = c - 'a' + 36;
      }
      else {
        v = 0;
      }

      //std::cerr << "i=" << i << ", c=" << c << ", v=" << v << std::endl;

      result += v * std::pow(62, len-i);
    }

    return result;
  }

  kvstore::kvstore(const std::string filename)
  {
    using namespace kyotocabinet;

    if (!db_.open(filename, HashDB::OWRITER | HashDB::OCREATE)) {
      throw std::runtime_error("kvstore open error");
    }

    if ( count() == 0 )
    {
      db_.increment("__artist_key__", base62_decode("ar0000"));
      db_.increment("__album_key__", base62_decode("al0000"));
      db_.increment("__track_key__", base62_decode("t00000"));
    }
  }

  kvstore::~kvstore()
  {
    db_.close();
  }

  int64_t kvstore::count()
  {
    return db_.count();
  }

  bool kvstore::set(const std::string& key, const json::value& value)
  {
    return db_.set(key, to_string(value));
  }

  bool kvstore::remove(const std::string& key)
  {
    return db_.remove(key);
  }

  json::value kvstore::get(const std::string& key)
  {
    std::string value_s;

    if ( db_.get(key, &value_s) )
    {
      json::value  value;
      json::parser parser(value);

      size_t consumed = parser.parse(value_s.c_str(), value_s.length());

      if ( consumed < value_s.length() )
      {
        std::cout << "error! retrieving value" << std::endl;
      }

      return std::move(value);
    }
    else
    {
      return json::value();
    }
  }

  void kvstore::each(std::function<bool(const std::string&)> key_match, std::function<bool(json::value&)> value_cb)
  {
    kyotocabinet::DB::Cursor* cur = db_.cursor();

    cur->jump();

    std::string ckey;

    while ( cur->get_key(&ckey, false) )
    {
      if ( key_match(ckey) )
      {
        std::string cvalue;

        if ( cur->get_value(&cvalue) )
        {
          json::value  value;
          json::parser parser(value);

          size_t consumed = parser.parse(cvalue.c_str(), cvalue.length());

          if ( consumed < cvalue.length() )
          {
            std::cout << "error! retrieving value" << std::endl;
          }

          value_cb(value);
        }
      }
      cur->step();
    }

    delete cur;
  }

  std::string kvstore::create_artist_key()
  {
    auto new_key = db_.increment("__artist_key__", 1);
    return std::move(base62_encode(new_key));
  }

  std::string kvstore::create_album_key()
  {
    auto new_key = db_.increment("__album_key__", 1);
    return std::move(base62_encode(new_key));
  }

  std::string kvstore::create_track_key()
  {
    auto new_key = db_.increment("__track_key__", 1);
    return std::move(base62_encode(new_key));
  }
}