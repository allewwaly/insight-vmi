# Building InSight from Source #



# Introduction #

The following instructions describe how to compile, install, and set up InSight from the source distribution.

# Obtaining the Source Code #

The source code of the latest stable version of InSight can be obtained from the [download page](https://code.google.com/p/insight-vmi/downloads/list). You can check out the latest, possibly instable version from the subversion repository as explained on the [source page](https://code.google.com/p/insight-vmi/source/checkout):

```sh
svn checkout http://insight-vmi.googlecode.com/svn/trunk/ insight-vmi```

It sometimes happens that the trunk version fails to build. In that case, you should be save with a tagged version (replace `0.4` with the latest version within the [tags directory](https://code.google.com/p/insight-vmi/source/browse/tags)):

```sh
svn checkout http://insight-vmi.googlecode.com/svn/tags/0.4/ insight-vmi```


# Building the Source #

The following instructions have been tested on a system with different 64-bit Linux distrubutions. This is the recommended host platform. Note that InSight is completely untested on 32-bit hosts. Feel free to give it a try on some 32-bit host and share your experience with us.

## Prerequisites ##

InSight is written in C++ using the Qt libraries and the QMake build system. The following packages are required in order to build InSight (package names might vary for different distrubutions):

  * g++
  * qt4-dev-tools
  * make
  * libqt4-dev
  * libreadline-dev

## Compilation and Installation ##

The source directory `insight-vmi` you just checked out before should contain at least the following directories and files:

<pre>
debian/<br>
insight/<br>
insightd/<br>
libantlr3c/<br>
libcparser/<br>
libdebug/<br>
libinsight/<br>
tools/<br>
insight-vmi.pro<br>
Makefile.manual<br>
</pre>

If your system fulfills the prerequisites, the source can be compiled by changing into the `insight-vmi` directory containing the above listed files and executing:

```sh
make -f Makefile.manual```

You can verify that the compilation succeeded by assuring that the following files exist:

<pre>
insight/insight<br>
insightd/insightd<br>
libinsight/libinsight.so<br>
</pre>

To install InSight in the `/usr/local` tree, use the following command:

```
sudo make -f Makefile.manual DESTDIR=/ PREFIX=/usr/local install &&
sudo ldconfig
```

You can also run InSight without installation, see [Running InSight](RunningInSight.md).

# Creating Debian Packages #

The source code comes with all required control files to create packages for Debian-based distributions such as Ubuntu. This requires an additional package to be installed:

  * dpkg-dev

Now instead of the `make` utility issue the following command in the source directory:

```sh
dpkg-buildpackage -us -uc -b```

This will compile the source and create `deb` packages in the directory one level above the current (the actual file names might differ depending on version and architecture):

<pre>
../insight_0.4_amd64.deb<br>
../insight-daemon_0.4_amd64.deb<br>
../insight-doc_0.4_all.deb<br>
../insight-scripts_0.4_amd64.deb<br>
../libinsight_0.4_amd64.deb<br>
../libinsight-dev_0.4_amd64.deb<br>
</pre>

You can install these packages [the same way as the packages we provide](InstallationOnDebian.md).