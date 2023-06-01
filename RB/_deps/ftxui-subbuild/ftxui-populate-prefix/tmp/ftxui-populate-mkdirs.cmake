# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/media/rafid/Resources/Projects/StockDory/RB/_deps/ftxui-src"
  "/media/rafid/Resources/Projects/StockDory/RB/_deps/ftxui-build"
  "/media/rafid/Resources/Projects/StockDory/RB/_deps/ftxui-subbuild/ftxui-populate-prefix"
  "/media/rafid/Resources/Projects/StockDory/RB/_deps/ftxui-subbuild/ftxui-populate-prefix/tmp"
  "/media/rafid/Resources/Projects/StockDory/RB/_deps/ftxui-subbuild/ftxui-populate-prefix/src/ftxui-populate-stamp"
  "/media/rafid/Resources/Projects/StockDory/RB/_deps/ftxui-subbuild/ftxui-populate-prefix/src"
  "/media/rafid/Resources/Projects/StockDory/RB/_deps/ftxui-subbuild/ftxui-populate-prefix/src/ftxui-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/media/rafid/Resources/Projects/StockDory/RB/_deps/ftxui-subbuild/ftxui-populate-prefix/src/ftxui-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/media/rafid/Resources/Projects/StockDory/RB/_deps/ftxui-subbuild/ftxui-populate-prefix/src/ftxui-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
