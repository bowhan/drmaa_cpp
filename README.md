# C++ Wrapper for DRMAA

## This is work still ongoing

This repository contains simple C++ wrapper for the DRMAA C library, optimized for SGE within [AWS Parallel Cluster](https://aws.amazon.com/blogs/opensource/aws-parallelcluster/).

## Pre-requisites

- Set enviromental variables

  ```bash
  export SGE_ROOT=/opt/sge
  export SGE_CELL=default
  ```

- Make `libdrmaa.so` available

  ```bash
  sudo ln -s /opt/sge/lib/lx-amd64/libdrmaa.so /usr/local/lib/libdrmaa.so
  ```

## Usage

See `tests/test.cpp`.
