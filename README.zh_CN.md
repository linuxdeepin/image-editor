# image-editor

Image editor 是深度开发的用于  deepin-image-viewer 和 deepin-album 的图像处理公共库。

### 依赖

### 编译依赖

_**master**分支是当前开发分支，编译依赖可能在未更新README.md文件的情况下变更，请参考./debian/control文件获取有效的编译依赖列表_

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

## 安装

### 构建过程

1. 确保已安装所有依赖库.

_不同发行版的软件包名称可能不同，如果您的发行版提供了image-editor，请检查发行版提供的打包脚本。_

如果你使用的是 [Deepin](https://distrowatch.com/table.php?distribution=deepin) 或者其它提供了image-editor的发行版:

``` shell
$ sudo apt-get build-dep image-editor
```
或者手动安装
``` shell
$ sudo apt install cmake pkg-config qtbase5-dev qtbase5-private-dev qttools5-dev qttools5-dev-tools qtmultimedia5-dev libqt5svg5-dev libqt5x11extras5-dev qt6-base-dev qt6-base-private-dev qt6-tools-dev qt6-tools-dev-tools qt6-multimedia-dev libqt6svg6-dev libqt6opengl6-dev libexif-dev libsqlite3-dev libxcb-util0-dev libstartup-notification0-dev libraw-dev x11proto-xext-dev libmtdev-dev libegl1-mesa-dev libudev-dev libfontconfig1-dev libfreetype6-dev libxrender-dev libdtkwidget-dev libdtkcore5-bin libdtk6widget-dev libdtk6core-bin libmediainfo-dev libffmpegthumbnailer-dev libtiff-dev libdfm6-io-dev libdfm-io-dev
```

2. 构建:

```
$ cd draw
$ mkdir Build
$ cd Build
$ cmake ../
$ make
```

3. 安装:

```
$ sudo make install
```

## 帮助

任何使用问题都可以通过以下方式寻求帮助:

* [Gitter](https://gitter.im/orgs/linuxdeepin/rooms)
* [IRC channel](https://webchat.freenode.net/?channels=deepin)
* [Forum](https://bbs.deepin.org)
* [WiKi](https://wiki.deepin.org/)

## 贡献指南

我们鼓励您报告问题并做出更改

* [Contribution guide for developers](https://github.com/linuxdeepin/developer-center/wiki/Contribution-Guidelines-for-Developers-en). 

## 开源许可证
Image editor 在 [GPL-3.0-or-later](LICENSE.txt) 下发布。