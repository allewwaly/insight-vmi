# Introduction #

This page describes how to install the Debian packages we provide for Debian, Ubuntu and other Debian-based Linux distributions.


# Installing the Packages from the Command Line #

First download at least the `libinsight`, `insight` and `insightd` packages from the [download page](https://code.google.com/p/insight-vmi/downloads/list). Note that InSight has only been tested on 64-bit hosts, so we only provide AMD64 binary packages. You may want to download the package `insight-scripts` with some sample scripts as well.

Install these packages with the `dpkg` package manager from the command line (the version may vary):

```sh
sudo dpkg -i \
libinsight_0.4_amd64.deb \
insight_0.4_amd64.deb \
insight-daemon_0.4_amd64.deb```

In the same way, you can install any other package you downloaded in addition.