FROM fedora:29 as base

RUN dnf install -y \
  https://download1.rpmfusion.org/free/fedora/rpmfusion-free-release-$(rpm -E %fedora).noarch.rpm \
  https://download1.rpmfusion.org/nonfree/fedora/rpmfusion-nonfree-release-$(rpm -E %fedora).noarch.rpm

RUN dnf install -y \
  boost \
  boost-devel \
  boost-static \
  cmake \
  ffmpeg \
  ffmpeg-devel \
  gcc \
  gcc-c++ \
  ninja-build

WORKDIR /opt/metamix-build

ADD CMakeModules /opt/metamix-src/CMakeModules
ADD vendor /opt/metamix-src/vendor

ADD CMakeLists.txt /opt/metamix-src/
ADD test /opt/metamix-src/test
ADD src /opt/metamix-src/src

RUN cmake -DCMAKE_BUILD_TYPE=Release -DCPACK_PACKAGE_FILE_NAME=metamix -G "Ninja" /opt/metamix-src && ninja


FROM fedora:29

RUN dnf install -y \
  https://download1.rpmfusion.org/free/fedora/rpmfusion-free-release-$(rpm -E %fedora).noarch.rpm \
  https://download1.rpmfusion.org/nonfree/fedora/rpmfusion-nonfree-release-$(rpm -E %fedora).noarch.rpm

RUN dnf install -y ffmpeg

COPY --from=base \
  /opt/metamix-build/metamix \
  /opt/metamix-build/metamix-tests \
  /usr/local/bin/

CMD metamix
