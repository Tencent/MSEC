<?php
$br = (php_sapi_name() == "cli")? "":"<br>";

if(!extension_loaded('log_php')) {
	dl('log_php.' . PHP_SHLIB_SUFFIX);
}

nglog_set_option('ServiceName', 'TestPhpSvc.TestPhpSvc');
nglog_debug("something debug.");
nglog_error("something error.");
?>
