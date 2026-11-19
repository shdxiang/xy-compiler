#!/bin/bash
set -e

sudo apt-get update
sudo apt-get install -y git build-essential cmake ninja-build flex bison llvm-dev \
    zlib1g-dev libzstd-dev libedit-dev libxml2-dev libffi-dev libncurses-dev
