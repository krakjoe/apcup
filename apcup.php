<?php
$br = (php_sapi_name() == "cli")? "":"<br>";

if(!extension_loaded('apcup')) {
	dl('apcup.' . PHP_SHLIB_SUFFIX);
}

echo "<pre>";
if (extension_loaded("apcup")) {
    /* registers constant id, given some params */
	var_dump(apcup_create("APCUP_DEFAULT", 2048, 7020, 7020));
	
	/* test setting in specific cache */
	var_dump(apcup_set(APCUP_DEFAULT, "test", $_SERVER));
    var_dump(apcup_get(APCUP_DEFAULT, "test"));
	var_dump(apcup_info(APCUP_DEFAULT));
	var_dump(apcup_clear(APCUP_DEFAULT));
	var_dump(apcup_info(APCUP_DEFAULT));
	/* test getting from specific cache */
} else die("canot load apcup");

print_r(get_extension_funcs("apcup"));
?>
