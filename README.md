nemocore is a linux based platform inspired by SF movies.

Our goal is to support multi-user and multi-input/out-put to bring futuristic experiences.

# Requirements
Below commands are exectuted under Ubuntu Linux environment.

We recommend Ubuntu server 16.10.

## Unofficlal debian packages

follow the link and install the package

* [skia](https://github.com/nemoux/skia_build)

# How to build and install

## Install dependent packages.

```
$sudo apt-get install devscripts equivs
$sudo mk-builddeps -r -i
```

## Make and Install

```
$mkdir build && cd build
$cmake .. && make && sudo make install
```

Install and enable our systemd services
```
$cd ..
$sudo cp -rfv debian/nemobusd.service /lib/systemd/system/
$sudo cp -rfv debian/minishell.service /lib/systemd/system/
```

## Create a debian package and Install it

This is more simple & easy than [Make and Install](#make-and-install)
```
$dpkg-buildpackage -b -uc -us
$sudo dpkg -i nemocore_xxx.deb
```

# How to execute minishell

```
$sudo systemctl enable nemobusd.service
$sudo systemctl enable minishell.service
$sudo systemctl start nemobusd.service
$sudo systemctl start minishell.service
```

# TODO

nothing yet.
