// ----------------------------------------------------------------------------
//
//     Filename   : track_base.h
//
//     Author     : Benny Bach <benny.bach@gmail.com>
//                  Copyright (C) 2014
//
// --- Description: -----------------------------------------------------------
//
//
// ----------------------------------------------------------------------------
#ifndef __track_base_h__
#define __track_base_h__

// ----------------------------------------------------------------------------
#include <json/json.h>

// ----------------------------------------------------------------------------
#include <iostream>
#include <string>
#include <set>
#include <vector>

// ----------------------------------------------------------------------------
struct track_source
{
  std::string name;
  std::string uri;
  json::value replaygain;
};

// ----------------------------------------------------------------------------
class track_base
{
public:
  typedef std::set<std::string>     tag_set_t;
  typedef std::vector<json::object> source_list_t;
public:
  track_base()
    :
    title_(),
    track_number_(-1),
    disc_number_(1),
    duration_(-1),
    rating_(-1),
    sources_()
  {
  }
public:
  void title(std::string title)
  {
    title_ = std::move(title);
  }
public:
  void track_number(int track_number)
  {
    track_number_ = track_number;
  }
public:
  void disc_number(int disc_number)
  {
    disc_number_ = disc_number;
  }
public:
  void duration(int duration_in_msecs)
  {
    duration_ = duration_in_msecs;
  }
public:
  void rating(double value)
  {
    rating_ = value;
  }
public:
  //virtual void artist(std::string artist) = 0;
  //virtual void album(std::string album) = 0;
public:
  void source_add(json::object source)
  {
    // Replace or add source.
    for ( auto& s : sources_ ) {
      if ( source["name"].as_string() == s["name"].as_string() ) {
        s = std::move(source);
      }
    }

    if ( !source.empty() ) {
      sources_.push_back(std::move(source));
    }
  }
public:
  const std::string& title() const
  {
    return title_;
  }
public:
  int track_number() const
  {
    return track_number_;
  }
public:
  int disc_number() const
  {
    return disc_number_;
  }
public:
  int duration() const
  {
    return duration_;
  }
public:
  double rating() const
  {
    return rating_;
  }
public:
  virtual const std::string& artist() const = 0;
  virtual const std::string& album() const = 0;
public:
  void tag_add(std::string tag)
  {
    tags_.insert(std::move(tag));
  }
public:
  void tag_remove(const std::string& tag)
  {
    tags_.erase(tag);
  }
public:
  const tag_set_t& tags() const
  {
    return tags_;
  }
public:
  void tags_clear()
  {
    tags_.clear();
  }
public:
  const source_list_t& sources() const
  {
    return sources_;
  }
public:
  track_source find_source(const std::string& source_name)
  {
    if ( source_name.length() > 0 )
    {
      // TODO:
      return track_source{};
    }
    else
    {
      return track_source{
        sources_[0]["name"].as_string(),
        sources_[0]["uri"].as_string(),
        sources_[0]["replaygain"]
      };
    }
  }
private:
  std::string   title_;
  int           track_number_;
  int           disc_number_;
  int           duration_;
  double        rating_;
  tag_set_t     tags_;
  source_list_t sources_;
};

// ----------------------------------------------------------------------------
#endif // __track_base_h__
