<?php

$x = 1;

if ($x > 1) echo 'Check value for X!' . PHP_EOL;

exit('Program ends...' . PHP_EOL);

/*
 * my php.ini debugger/profiler settings:
 *
[xdebug]
zend_extension="/usr/lib/php/20170718/xdebug.so"
xdebug.remote=1
xdebug.remote_enable=1
xdebug.remote_host=namaste
xdebug.remote_port=9000
xdebug.remote_autostart=0
xdebug.var_display_max_depth=10
xdebug.cli_color=1
xdebug.remote_log="/home/logs/xdebug.log"
;xdebug.profiler_enable=1
;xdebug.profiler_output_dir="/home/logs"
;xdebug.profiler_output_name="cachegrind.out.%p"
;xdebug.profiler_enable_trigger=1
 *
 */
