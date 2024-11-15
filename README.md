# image-editor

Image editor is a public library for deepin-image-viewer and deepin-album  developed by Deepin Technology.

### Dependencies

### Build dependencies

_The **master** branch is current development branch, build dependencies may changes without update README.md, refer to `./debian/control` for a working build depends list_

* debhelper (>= 11), 
* cmake, 
* pkg-config,
* qtbase5-dev,
* qtbase5-private-dev,
* qttools5-dev,
* qttools5-dev-tools,
* qtmultimedia5-dev,
* libqt5svg5-dev,
* libqt5x11extras5-dev,
* qt6-base-dev,
* qt6-base-private-dev,
* qt6-tools-dev,
* qt6-tools-dev-tools,
* qt6-multimedia-dev, 
* libqt6svg6-dev,
* libqt6opengl6-dev,
* libexif-dev, 
* libsqlite3-dev, 
* libxcb-util0-dev, 
* libstartup-notification0-dev,
* libraw-dev, 
* x11proto-xext-dev, 
* libmtdev-dev, 
* libegl1-mesa-dev,
* libudev-dev, 
* libfontconfig1-dev, 
* libfreetype6-dev, 
* libxrender-dev, 
* libdtkwidget-dev,
* libdtkcore5-bin,
* libdtk6widget-dev,
* libdtk6core-bin,
* libmediainfo-dev,
* libffmpegthumbnailer-dev,
* libtiff-dev,
* libdfm6-io-dev | hello,
* libdfm-io-dev | hello

## Installation

### Build from source code

1. Make sure you have installed all dependencies.

_Package name may be different between distros, if image-editor is available from your distro, check the packaging script delivered from your distro is a better idea._

Assume you are using [Deepin](https://distrowatch.com/table.php?distribution=deepin) or other debian-based distro which got image-editor delivered:

``` shell
$ sudo apt-get build-dep image-editor
```
or
``` shell
$ sudo apt install cmake pkg-config qtbase5-dev qtbase5-private-dev qttools5-dev qttools5-dev-tools qtmultimedia5-dev libqt5svg5-dev libqt5x11extras5-dev qt6-base-dev qt6-base-private-dev qt6-tools-dev qt6-tools-dev-tools qt6-multimedia-dev libqt6svg6-dev libqt6opengl6-dev libexif-dev libsqlite3-dev libxcb-util0-dev libstartup-notification0-dev libraw-dev x11proto-xext-dev libmtdev-dev libegl1-mesa-dev libudev-dev libfontconfig1-dev libfreetype6-dev libxrender-dev libdtkwidget-dev libdtkcore5-bin libdtk6widget-dev libdtk6core-bin libmediainfo-dev libffmpegthumbnailer-dev libtiff-dev libdfm6-io-dev libdfm-io-dev
```

2. Build:

```
$ cd image-editor
$ mkdir Build
$ cd Build
$ cmake ../
$ make
```

3. Install:

```
$ sudo make install
```

## Getting help

Any usage issues can ask for help via

* [Gitter](https://gitter.im/orgs/linuxdeepin/rooms)
* [IRC channel](https://webchat.freenode.net/?channels=deepin)
* [Forum](https://bbs.deepin.org)
* [WiKi](https://wiki.deepin.org/)

## Getting involved

We encourage you to report issues and contribute changes

* [Contribution guide for developers](https://github.com/linuxdeepin/developer-center/wiki/Contribution-Guidelines-for-Developers-en). 

## License
Image editor is licensed under [GPL-3.0-or-later](LICENSE.txt)