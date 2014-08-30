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

# -----------------------------------------------------------------------------
spec = Rake::ExecutableSpecification.new do |s|
    s.name = 'spotd'
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
        src/**/*.cpp
    )
    s.libraries += [ popt, json ] + %w(asound FLAC++ tag spotify b64)
end

# -----------------------------------------------------------------------------
Rake::ExecutableTask.new(:spotd, spec)

# -----------------------------------------------------------------------------
spec = Rake::ExecutableSpecification.new do |s|
    s.name = 'test'
    s.includes.add %w(
        src
        lib/json/include
        test/catch
    )
    s.libincludes.add %w(
        build
    )
    s.sources.add %w(
        src/local_source.cpp
        test/**/*.cpp

    )
    s.libraries += [ json ] + %w(asound FLAC++ tag b64)
end

# -----------------------------------------------------------------------------
Rake::ExecutableTask.new(:test, spec)

# -----------------------------------------------------------------------------
CLEAN.include('build')
# -----------------------------------------------------------------------------
task :default => [ :spotd ]
task :all => [ :default ]
