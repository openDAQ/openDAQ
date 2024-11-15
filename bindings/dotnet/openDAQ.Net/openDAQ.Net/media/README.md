# openDAQ.Net ![openDAQ](.\openDAQ-NuGet_128.png)

This NuGet package contains the .NET-Bindings to the openDAQ<sup>TM</sup> SDK as well as
the SDK itself.

## Table of Contents
1. [Development environment](#development-environment)
1. [Prerequisites](#prerequisites)
1. [Links](#links)


## Development environment
- Visual Studio 2022 (V17.11.x when this package has been created)
- TargetFramework: net6.0
- Platform: x64

## Prerequisites
Install this NuGet package for your project using NuGet Manager.  
On NuGet package restoration all relevant binaries will be copied to the configuration's
target directory and adapts the file `*.deps.json` to find the libraries since they're
being structured (e.g. `runtimes/win-x64/native/*`).

## Links
- [openDAQ](https://opendaq.com/)
