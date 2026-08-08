# File Manager

A lightweight desktop file manager built in C++ with [wxWidgets](https://www.wxwidgets.org/).

## Features

- Browse a directory by typing a path into the address bar
- List files and directories with a report-style view
- Double-click to open a file or navigate into a directory
- Copy, cut, and paste files
- Rename, delete, and create new directories
- Refresh the current directory listing

## Requirements

- A C++17 compiler (e.g. clang++ or g++)
- [wxWidgets 3.3.1](https://www.wxwidgets.org/downloads/) built or installed on your system

This repository does not include the wxWidgets source — download it separately and point your build at its headers/libraries.

## Building

```bash
g++ main.cpp -o app `wx-config --cxxflags --libs`
```

Adjust the `wx-config` path if wxWidgets isn't on your `PATH`, or use the `.vscode` build task included in this repo.

## Running

```bash
./app
```
