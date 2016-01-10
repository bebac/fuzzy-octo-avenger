# -----------------------------------------------------------------------------
require 'rake/clean'
require 'rake/tasklib'

# -----------------------------------------------------------------------------
require './rakelib/lib/ctasklib'

# -----------------------------------------------------------------------------
case ENV["variant"]
when "release"
    ENV["CFLAGS"]  = %q(-O2 -Wall -MMD)
    ENV["LDFLAGS"] = %q(-pthread)
else
    ENV["CFLAGS"]  = %q(-g -Wall -MMD)
    ENV["LDFLAGS"] = %q(-g -pthread)
end

ENV["CPPFLAGS"] = %q(-std=c++11)

# -----------------------------------------------------------------------------
popt = Rake::StaticLibraryTask.new("lib/program-options/program-options.yml")
json = Rake::StaticLibraryTask.new("lib/json/json.yml")
dc   = Rake::StaticLibraryTask.new("lib/dripcore/dripcore.yml")
dm   = Rake::StaticLibraryTask.new("src/dm/dm.yml")

# -----------------------------------------------------------------------------
spec = Rake::ExecutableSpecification.new do |s|
    s.name = 'mboxd'
    s.includes.add %w(
        src
        lib/program-options/include
        lib/json/include
        lib/dripcore/include
        lib/libspotify-12.1.51-Linux-x86_64-release/include
    )
    s.libincludes.add %w(
        build
        lib/libspotify-12.1.51-Linux-x86_64-release/lib
    )
    s.sources.add %w(
        src/*.cpp
    )
    s.libraries += [ popt, dm, dc, json ] + %w(asound FLAC++ tag spotify kyotocabinet)
end

# -----------------------------------------------------------------------------
Rake::ExecutableTask.new(:mboxd, spec)

# -----------------------------------------------------------------------------
spec = Rake::ExecutableSpecification.new do |s|
    s.name = 'test'
    s.includes.add %w(
        src
        lib/json/include
        lib/dripcore/include
        test/catch
    )
    s.libincludes.add %w(
        build
        buildimport_tracks
    )
    s.sources.add %w(
        src/local_source.cpp
        src/base64.cpp
        test/**/*.cpp

    )
    s.libraries += [ dm, dc, json ] + %w(asound FLAC++ tag kyotocabinet)
end

# -----------------------------------------------------------------------------
Rake::ExecutableTask.new(:test, spec)

# -----------------------------------------------------------------------------
namespace :json do
    task :update do
        sh "git subtree pull --prefix lib/json/ https://github.com/bebac/json-cpp11-library.git master --squash"
    end
end

# -----------------------------------------------------------------------------
CLEAN.include('build')
# -----------------------------------------------------------------------------
task :default => [ :mboxd ]
task :all => [ :default ]
