#!/bin/zsh

compdef _exmkfs exmkfs

function _exmkfs() {
    _arguments '--device[device name]:filename:_files' \
        '--inodes[number of inodes]:number:' \
        '--size[size of a device]:size:' \
        '--create[create device]' \
        '--log-level[log level]:level:(debug info warning error fatal)'
}
