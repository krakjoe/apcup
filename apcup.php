<?php
$br = (php_sapi_name() == "cli")? "":"<br>";

if(!extension_loaded('apcup')) {
	dl('apcup.' . PHP_SHLIB_SUFFIX);
}

echo "<pre>";
if (extension_loaded("apcup")) {
    /* registers constant id, given some params */
	var_dump(apcup_create("APCUP_DEFAULT", 2048, 7020, 7020));
    var_dump(apcup_create("APCUP_OTHER", 2048, 7020, 7020));
    
	var_dump(APCUP_DEFAULT);
	var_dump(APCUP_OTHER);
	
	/* test setting in specific cache */
	if ($_REQUEST["set"]) {
	    apcup_set(APCUP_DEFAULT, "default", $_REQUEST);
	    apcup_set(APCUP_OTHER,   "other", $_SERVER);
	}

	var_dump(apcup_get(APCUP_OTHER, "other"));
    var_dump(apcup_get(APCUP_DEFAULT, "default"));
	
	var_dump(apcup_info(APCUP_OTHER));
	var_dump(apcup_info(APCUP_DEFAULT));
	/* test getting from specific cache */
} else die("canot load apcup");
?>
