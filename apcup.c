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

/* $Id$ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_apcup.h"

ZEND_DECLARE_MODULE_GLOBALS(apcup)

/* {{{ globals */
static apcup_t apcup; /* }}} */

/* {{{ quick accessor */
#define AP(id) \
    apcup.list[id] /* }}} */

/* {{{ quick accessor */
#define AP_CACHE(id) \
    AP(id)->cache /* }}} */
    
/* {{{ quick accessor */
#define AP_NAME(id) \
    AP(id)->name /* }}} */
    
/* {{{ quick tester */
#define AP_IS_CACHE(id) (id < apcup.next) /* }}} */

/* {{{ apcup_expunge: run when apcups is low on memory */
void apcup_gc(apcup_cache_t** list TSRMLS_DC) {
    zend_error(
        E_ERROR, "gc is not implemented yet, we are testing !!");
} /* }}} */

/* {{{ implement sma */
apc_sma_api_impl(apcups, &apcup.list, apcup_gc); /* }}} */

ZEND_BEGIN_ARG_INFO_EX(apcup_create_arginfo, 0, 0, 1)
    ZEND_ARG_INFO(0, name)
    ZEND_ARG_INFO(0, entries_hint)
    ZEND_ARG_INFO(0, gc_ttl)
    ZEND_ARG_INFO(0, ttl)
    ZEND_ARG_INFO(0, smart)
    ZEND_ARG_INFO(0, slam_defense)
ZEND_END_ARG_INFO()

/* {{{ apcup_functions[] */
const zend_function_entry apcup_functions[] = {
	PHP_FE(apcup_create, apcup_create_arginfo)
	PHP_FE(apcup_get,    NULL)
	PHP_FE(apcup_set,    NULL)
	PHP_FE(apcup_info,    NULL)
	PHP_FE(apcup_clear,  NULL)
	PHP_FE_END
}; /* }}} */

/* {{{ apcup_module_entry
 */
zend_module_entry apcup_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
	STANDARD_MODULE_HEADER,
#endif
	"apcup",
	apcup_functions,
	PHP_MINIT(apcup),
	PHP_MSHUTDOWN(apcup),
	NULL,
	NULL,
	PHP_MINFO(apcup),
#if ZEND_MODULE_API_NO >= 20010901
	"0.1", /* Replace with version number for your extension */
#endif
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_APCUP
ZEND_GET_MODULE(apcup)
#endif

/* {{{ PHP_INI
 */
PHP_INI_BEGIN()
    /*
    * This is not adjustable at runtime
    */
    STD_PHP_INI_ENTRY("apcup.shared", "32", PHP_INI_SYSTEM, OnUpdateLong, shared, zend_apcup_globals, apcup_globals)
    
    /* other ini entries here */
PHP_INI_END()
/* }}} */

/*
 * apcup_cache_id
 * returns the id of a previously registered cache
 * assumes you hold a lock on the cache list while this occurs
 * will return 0L if not found
 */
static inline int apcup_cache_id(char* name, zend_uint nlength TSRMLS_DC) {
    int current = 0, end = apcup.next;
    {
        while (current < end) {
            if (strncmp(name, AP_NAME(current), nlength) == SUCCESS) {
                return current;
            }
            
            current++;
        }
    }
    return -1;
}

/*
 * apcup_create_cache
 * returns a constant id for a cache
 * if the cache does not exist, it is registered
 * if the cache exists, the constant id of the cache will be returned
 * in any case of failure a zero value is returned
 *
 * @name the name of the cache, something constant friendly
 * @nlength the length of the data in name
 * @gc_ttl the gc ttl for this specific cache
 * @ttl the ttl for this specific cache
 * @smart the smart value for this specific cache
 * @slam_defense enable/disable slam defense for this specific cache
 */
static inline zend_bool apcup_create_cache(char *name, 
                                            zend_uint nlength,
                                            zend_ulong entries_hint,
                                            zend_ulong gc_ttl,
                                            zend_ulong ttl,
                                            zend_ulong smart,
                                            zend_bool slam_defense TSRMLS_DC) {
    zend_bool result = 0;
    
    int id = 0;
    
    APC_LOCK(&apcup);
    
    /* ensure this cache is not created twice */
    id = apcup_cache_id(name, nlength TSRMLS_CC);
    
    /* continue if there is no result and is a list */
	if (id == -1) {
	    
	    /* increment id */
	    apcup.list[apcup.next] = apcups.malloc(sizeof(apcup_cache_t) TSRMLS_CC);
	    
		if (apcup.list[apcup.next]) {
			/* create cache */
			apcup.list[apcup.next]->cache = apc_cache_create(
			    &apcups, 
			    NULL, /* TODO XXX no serializer support, we are only testing */ 
			    entries_hint, 
			    gc_ttl, ttl, 
			    smart, 
			    slam_defense TSRMLS_CC
			);
			
			/* check we are not failures */
			if (!apcup.list[apcup.next]->cache) {
                goto failure;
			}
			
			/* set name */
			apcup.list[apcup.next]->name = apc_xmemcpy(
				name, nlength, apcups.malloc TSRMLS_CC);
			apcup.list[apcup.next]->nlength = nlength;
			
			/* register constant id for cache as user */
			zend_register_long_constant(
			    apcup.list[apcup.next]->name, 
			    apcup.list[apcup.next]->nlength+1, 
			    apcup.next, 
			    CONST_CS, PHP_USER_CONSTANT TSRMLS_CC
			);
			
			/* set result */
			result = 1;
			
			/* move forward */
			apcup.next++;
		}
	} else {
	    /* register constant id for cache as user */
	    zend_register_long_constant(name, nlength+1, id, CONST_CS, PHP_USER_CONSTANT TSRMLS_CC);
	    
	    result = 1;
	}
	
	APC_UNLOCK(&apcup);
	
	return result;

failure:
    zend_error(
        E_ERROR, 
        "APCu failed to create the requested cache (%s), do you have enough resources ?",
        name
    );
    APC_UNLOCK(&apcup);
    
    return 0;
}

/*
* Startup APCu Pooling
*/
static inline zend_bool apcup_startup(zend_uint mod TSRMLS_DC) {
    /* make sure we do not initialize twice */
	if (++apcup.refcount > 1) 
		return 1;
		
    /* set module number */
    apcup.mod = mod;

    /* set next id to nothing */
	apcup.next = 0;    
    
    /* initialize locking */
    apc_lock_init(TSRMLS_C);

    /* initialize shared memory */
    apcups.init(
        1, 1024 * 1024 * APG(shared), NULL TSRMLS_CC);
    
    /* create a lock for safety */
    CREATE_LOCK(&apcup.lock);

	/* allocate list structure */
    apcup.list = apcups.malloc(sizeof(apcup_cache_t**) TSRMLS_CC);
    
    /* indicate we failed */
    if (!apcup.list)
        return 0;

	return 1;
}

/*
* apcup_shutdown
*/
static inline void apcup_shutdown(TSRMLS_D) {
    /* apcup shutdown */
	APC_LOCK(&apcup);
    {
        if (--apcup.refcount == 0) {
            int current = 0, end = apcup.next;
            
            while (current < end) {
                apc_cache_destroy(apcup.list[current]->cache TSRMLS_CC);
                if (apcup.list[current]->name) {
                     apcups.free(apcup.list[current]->name TSRMLS_CC);
                }
                current++;
            }
            
            /* free list */
            apcups.free(apcup.list TSRMLS_CC);
            
            /* null items */
            apcup.list = NULL;
        } else goto nothing;
    }
    APC_UNLOCK(&apcup);

    /* destroy this */
    DESTROY_LOCK(&apcup.lock);
    
    /* cleanup sma */
    apcups.cleanup(TSRMLS_C);
    
    return;
    
nothing:
    APC_UNLOCK(&apcup);
    
    return;
}

/* {{{ php_apcup_init_globals
 */
static void php_apcup_init_globals(zend_apcup_globals *apcup_globals)
{
	apcup_globals->shared = 32;
}
/* }}} */

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(apcup)
{
	REGISTER_INI_ENTRIES();
    
    if (!apcup_startup(module_number TSRMLS_CC))
		zend_error(E_ERROR, "APCu pooling failed to startup, try raising apcup.shared");

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(apcup)
{
	UNREGISTER_INI_ENTRIES();

    apcup_shutdown(TSRMLS_C);

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(apcup)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "apcup support", "enabled");
	php_info_print_table_end();

	DISPLAY_INI_ENTRIES();
}
/* }}} */

/* {{{ proto bool apcup_create(string name [, long entries_hint = 1024, [ long gc_ttl = 0, [ long ttl = 0, [ long smart = 0, [ bool slam_defense = true]]]]]) 
   Create a cache and register a constant identifier for future operations 
   Should the cache already exist, no action is performed */
PHP_FUNCTION(apcup_create) 
{
    char *name = NULL;
    zend_uint nlength = 0L;
    zend_ulong entries_hint = 1024;
    zend_ulong gc_ttl = 0L;
    zend_ulong ttl = 0L;
    zend_ulong smart = 0L;
    zend_bool slam_defense = 1;
    
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|llllb", &name, &nlength, &entries_hint, &gc_ttl, &ttl, &smart, &slam_defense) != SUCCESS) {
        return;
    }
    
    RETURN_BOOL(apcup_create_cache(name, nlength, entries_hint, gc_ttl, ttl, smart, slam_defense TSRMLS_CC));
} /* }}} */

/* {{{ proto boolean apcup_set(long cache, string name, mixed value, [, long ttl])
   Sets a value in the specific cache 
   Returns true on success */
PHP_FUNCTION(apcup_set)
{
    zend_ulong cache = 0L;
    char *key = NULL;
    zend_uint klen = 0L;
    zend_uint ttl = 0L;
    zval *pzval = NULL;
    
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "lsz|l", &cache, &key, &klen, &pzval, &ttl) != SUCCESS) {
        return;
    }
    
    if (AP_IS_CACHE(cache)) {
        ZVAL_BOOL(return_value, apc_cache_store(AP_CACHE(cache), key, klen+1, pzval, ttl, 1 TSRMLS_CC));  
    } else zend_error(E_WARNING, "APCu could not find the requested cache (%d)", cache);
} /* }}} */

/* {{{ proto mixed apcup_get(long cache, string name) 
  Get a value from a specific cache */
PHP_FUNCTION(apcup_get)
{
    zend_ulong cache;
    char *key;
    zend_uint klen;
    
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ls", &cache, &key, &klen) != SUCCESS) {
        return;
    }
    
    if (AP_IS_CACHE(cache)) {
       if (!apc_cache_fetch(AP_CACHE(cache), key, klen+1, time(0), &return_value TSRMLS_CC)) {
            /* not found */
        } 
    } else zend_error(E_WARNING, "APCu could not find the requested cache (%d)", cache);
} /* }}} */

/* {{{ proto mixed apcup_info(long cache) 
  Get info about cache */
PHP_FUNCTION(apcup_info)
{
    zend_ulong cache;
    
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &cache) != SUCCESS) {
        return;
    }
    
    if (AP_IS_CACHE(cache)) {
       zval* info = apc_cache_info(AP_CACHE(cache), 0 TSRMLS_CC);
       
       if (info) {
           ZVAL_ZVAL(return_value, info, 1, 1);
       } else RETURN_NULL();
    }
} /* }}} */

/* {{{ proto void apcup_clear(long cache) 
   Clear a specific cache */
PHP_FUNCTION(apcup_clear)
{
    zend_ulong cache;
    
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &cache) != SUCCESS) {
        return;
    }
    
    if (AP_IS_CACHE(cache)) {
        apc_cache_clear(AP_CACHE(cache) TSRMLS_CC);
    }
} /* }}} */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
