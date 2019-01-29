#!/bin/zsh

compdef _exdbg exdbg

function _exdbg() {
    _arguments '--device[device name]:filename:_files' \
        '--inode[inode address]:address:' \
        '--inode-data[inode address]:address:' \
        '--bitmap-data[bitmap address]:address:' \
        '--super[print super block]' \
        '--info[display fs info]'
}
