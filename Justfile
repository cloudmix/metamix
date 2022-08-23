# Build & Test Metamix on source file change.
watch: vsc
  #!/usr/bin/env bash
  while :; do
    rg --files | entr -d just _watch_step
  done

_watch_step:
  cmake --build ./cmake-build-vsc/ -- -j 8
  ./cmake-build-vsc/metamix-tests -x -p

# Format C/C++ source code.
fmt:
  rg --files | rg '(src|test)/.*(h|hpp|c|cpp)$' | xargs clang-format -i

# (Re)create release build directory & build release packages.
publish:
  mkdir -p cmake-build-publish
  cd cmake-build-publish && cmake -DCMAKE_BUILD_TYPE=Release -G Ninja ..
  cd cmake-build-publish && cmake --build .
  cd cmake-build-publish && cpack

# (Re)create development build directory.
vsc:
  mkdir -p cmake-build-vsc
  cd cmake-build-vsc && cmake -DCMAKE_BUILD_TYPE=Debug ..

# Clean all build directories.
clean:
  rm -rf cmake-build-*/
