/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 2013 The PHP Group                                     |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author: Joe Watkins <joe.watkins@live.co.uk>                         |
  +----------------------------------------------------------------------+
*/

/*
* APCu Pools: Allow users to pool caches in user land
*/

/* $Id$ */

#ifndef PHP_APCUP_H
#define PHP_APCUP_H

extern zend_module_entry apcup_module_entry;
#define phpext_apcup_ptr &apcup_module_entry

#define PHP_APCUP_EXTNAME "apcup"
#define PHP_APCUP_VERSION "1.0.0"

#ifdef PHP_WIN32
#	define PHP_APCUP_API __declspec(dllexport)
#elif defined(__GNUC__) && __GNUC__ >= 4
#	define PHP_APCUP_API __attribute__ ((visibility("default")))
#else
#	define PHP_APCUP_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

#include "apcu/apc_api.h"

/* {{{ structure definition:  apcup_cache_t */
typedef struct  apcup_cache_t  apcup_cache_t;
struct apcup_cache_t {
    /* unique id */
    int              id;
    /* name data (copied) */
    char*            name;
	size_t           nlength;
	/* APCu cache */
	apc_cache_t      cache;
}; /* }}} */

/* {{{ structure definition: apcup_meta_t */
typedef struct apcup_meta_t {
    /* lock for object */
    apc_lock_t       lock;
    /* end of cache list */
    int              end;
} apcup_meta_t; /* }}} */

/* {{{ struct definition: apcup_t */
typedef struct apcup_t {
    /* shared memory */
    void*            shm;
    /* shared meta */
    apcup_meta_t*    meta;
	/* shared caches */
    apcup_cache_t**  list;
} apcup_t; /* }}} */

/* {{{ apcup shared memory */
apc_sma_api_decl(apcups); /* }}} */

/* {{{ module stuff */
PHP_RINIT_FUNCTION(apcup);
PHP_MINIT_FUNCTION(apcup);
PHP_MSHUTDOWN_FUNCTION(apcup);
PHP_MINFO_FUNCTION(apcup); /* }}} */

/* {{{ user functions for creation and manipulation of caches */
PHP_FUNCTION(apcup_create);
PHP_FUNCTION(apcup_get);
PHP_FUNCTION(apcup_set);
PHP_FUNCTION(apcup_info);
PHP_FUNCTION(apcup_clear); /* }}} */

ZEND_BEGIN_MODULE_GLOBALS(apcup)
	/*
	* the size of a shared segment
	* Default: (32MB)
	*/
	zend_long    shared;
	/*
	* the number of segments to use
	* Default: (1)
	*/
	zend_long    segments;
	/*
	* mask for mmap
	* Default: (null)
	*/
	char*        mask;
	/*
	* maximum number of caches expected
	* Default: (8)
	*/
	zend_long    caches;
	/*
	* enable/disable apcup in cli
	* Default: (true)
	*/
	zend_bool    cli;
	
	/*
	* The remainder of globals are used internally
	*/
	zend_bool    initialized;
ZEND_END_MODULE_GLOBALS(apcup)

#ifdef ZTS
#define APG(v) TSRMG(apcup_globals_id, zend_apcup_globals *, v)
#else
#define APG(v) (apcup_globals.v)
#endif

#if defined(ZTS) && defined(COMPILE_DL_APCUP)
ZEND_TSRMLS_CACHE_EXTERN();
#endif

#endif	/* PHP_APCUP_H */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
