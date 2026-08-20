cmake_minimum_required(VERSION 3.16)
project(GlassBarPlugins)

find_package(DD4hep REQUIRED)
include(DD4hepBuild)

dd4hep_add_plugin(GlassBarPlugins
  SOURCES src/*.cpp
  USES DD4hep::DDCore DD4hep::DDG4
)
