#!/bin/zsh

compdef _exfuse exfuse

function _exfuse() {
    _arguments '--device[device name]:filename:_files' \
        '--log-level[log level]:level:(debug info warning error fatal)' \
        '::mount mount:_files'
}
