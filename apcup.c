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
apcup_t* apcup = NULL; /* }}} */

/* {{{ quick tester */
#define AP_IS_CACHE(id) (id > 0 && id < apcup->meta->nid) /* }}} */

/* {{{ apcup_expunge: run when apcups is low on memory */
void apcup_gc(apcup_t* apcup TSRMLS_DC) {
    zend_error(
        E_ERROR, "gc is not implemented yet, we are testing !!");
} /* }}} */

/* {{{ implement sma */
apc_sma_api_impl(apcups, &apcup, apcup_gc); /* }}} */

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
    * These are not adjustable at runtime
    */
    STD_PHP_INI_ENTRY("apcup.shared",    "32",    PHP_INI_SYSTEM, OnUpdateLong,    shared,    zend_apcup_globals, apcup_globals)
    STD_PHP_INI_ENTRY("apcup.segments",  "1",     PHP_INI_SYSTEM, OnUpdateLong,    segments,  zend_apcup_globals, apcup_globals)
    STD_PHP_INI_ENTRY("apcup.mask",      NULL,    PHP_INI_SYSTEM, OnUpdateString,  mask,      zend_apcup_globals, apcup_globals)
    
    STD_PHP_INI_ENTRY("apcup.caches",    "8",     PHP_INI_SYSTEM, OnUpdateLong,    caches,    zend_apcup_globals, apcup_globals)
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
    int current = 0, end = apcup->meta->nid;
    {
        while (current < end) {
            if (apcup->list[current]) {
                if (strncmp(name, apcup->list[current]->name, nlength) == SUCCESS) {
                    return apcup->list[current]->id;
                }
                current++;
            } else break;
        }
    }
    return -1;
}

/*
 * apcup_cache_find
 * find a previously registered cache by id
 */
static inline apcup_cache_t* apcup_cache_find(zend_uint id TSRMLS_DC) {
    int current = 0, end = apcup->meta->nid;
    {
        while (current < end) {
            if (apcup->list[current]->id == id) {
                return apcup->list[current];
            }
            current++;
        }
    }
    return NULL;
}

/*
 * apcup_create_cache
 * returns indication of success
 * if the cache does not exist, it is registered
 * if the cache exists, it's constant id will be registered in this context
 * in any case of failure false is returned
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
    
    HANDLE_BLOCK_INTERRUPTIONS();
    
    APC_LOCK(apcup->meta);
    
    /* ensure we are within limits */
    if (apcup->meta->nid < apcup->meta->max) {
    
        /* ensure this cache is not created twice */
        id = apcup_cache_id(name, nlength TSRMLS_CC);
        
        /* continue if there is no result */
	    if (id == -1) {
	        
	        /* allocate new pooled cache object in shm */
	        apcup_cache_t* create = apcups.malloc(sizeof(apcup_cache_t) TSRMLS_CC);
	        
		    if (create) {
			    /* create cache */
			    {
			        /* apc allocates using local malloc() */
			        apc_cache_t* apc = apc_cache_create(
			            &apcups,
			            NULL, /* TODO XXX no serializer support, we are only testing */ 
			            entries_hint,
			            gc_ttl, ttl,
			            smart,
			            slam_defense TSRMLS_CC
			        );
			        
			        if (apc) {
			            /* copy to shm */
			            create->cache = *apc;
			            /* free original pointer */
			            apc_efree(apc);
			        } else goto failure;
			    }
			
			    /* set name */
			    create->name = apc_xmemcpy(
				    name, nlength, apcups.malloc TSRMLS_CC);
			    create->nlength = nlength;
			
			    /* set id */
			    create->id = (apcup->meta->nid)++;
			
			    /* register constant id for cache as user */
			    zend_register_long_constant(
			        create->name, 
			        create->nlength+1, 
			        create->id, 
			        CONST_CS, PHP_USER_CONSTANT TSRMLS_CC
			    );
			
			    /* set result */
			    result = 1;
			
			    /* set position */
			    apcup->list[create->id - 1] = create;
		    }
	    } else {
	        /* register constant id for cache as user */
	        zend_register_long_constant(
	            name, nlength+1, 
	            id, 
	            CONST_CS, PHP_USER_CONSTANT TSRMLS_CC
	        );
	        
	        result = 1;
	    }
    } else {
        zend_error(
            E_WARNING, 
            "APCu cannot create any more caches, consider raising the maximum (%d)",
            apcup->meta->max
        );
    }
	
	APC_UNLOCK(apcup->meta);
	
	HANDLE_UNBLOCK_INTERRUPTIONS();
	
	return result;

failure:
    zend_error(
        E_WARNING, 
        "APCu failed to create the requested cache (%s), do you have enough resources ?",
        name
    );
    APC_UNLOCK(apcup->meta);
    
    HANDLE_UNBLOCK_INTERRUPTIONS();
    
    return 0;
}

/*
* Startup APCu Pooling
*/
static inline zend_bool apcup_startup(zend_uint mod TSRMLS_DC) {
    if (APG(shared)) {
        if (!APG(initialized)) {
            /* only once */
            APG(initialized) = 1;
            
            /* main object initialization */
            apcup = apc_emalloc(sizeof(apcup_t) TSRMLS_CC);
            
            if (apcup) {
            
                /* initialize locking */
                apc_lock_init(TSRMLS_C);

                /* initialize sma */
                apcups.init(
                    APG(segments), 1024 * 1024 * APG(shared), APG(mask) TSRMLS_CC);
                
                /* set size */
                apcup->size = (sizeof(apcup_cache_t*) * APG(caches)) + sizeof(apcup_meta_t);
                
                /* allocate shm */
                apcup->shm = apcups.malloc(apcup->size TSRMLS_CC);
                
                if (apcup->shm) {
                    /* zero shm, makes for easier debug */
                    memset(apcup->shm, 0, apcup->size);
                
                    /* set meta at start of shm */
                    apcup->meta = (apcup_meta_t*) apcup->shm;
                    
                    if (apcup->meta) {
                        /* create a lock for safety */
                        CREATE_LOCK(&apcup->meta->lock);
                        
                        /* set next id for cache */
                        apcup->meta->nid = 1;
                        
                        /* maximum number of caches */
                        apcup->meta->max = APG(caches);
                        
	                    /* point list at end of meta */
	                    apcup->list = (apcup_cache_t**) (((char*) apcup->shm) + sizeof(apcup_meta_t));
	                    
	                    return 1;
                    } else return 0;
                } else return 0;
            } else return 0;
        } else return 1;
    } else return 1;
}

/*
* apcup_shutdown
*/
static inline void apcup_shutdown(TSRMLS_D) {
    if (apcup && apcup->meta) {
        /* apcup shutdown */
	    APC_LOCK(apcup->meta);
        {
            if (APG(initialized)) {
                /* destroy caches */
                {
                    int current = 0, end = apcup->meta->nid;
                    
                    while (current < end) {
                        if (apcup->list[current]) {
                            DESTROY_LOCK(&apcup->list[current]->cache.header->lock);
                        } else break;
                        
                        current++;
                    }
                }
                /* only once */
                APG(initialized) = 0;
            }
        }
        APC_UNLOCK(apcup->meta);
        return;
    }
}

/* {{{ php_apcup_init_globals
 */
static void php_apcup_init_globals(zend_apcup_globals *apcup_globals)
{
	apcup_globals->shared = 16;
	apcup_globals->mask = NULL;
	
	apcup_globals->initialized = 0;
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
    if (APG(shared) && APG(initialized)) {
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
    }
} /* }}} */

/* {{{ proto boolean apcup_set(long cache, string name, mixed value, [, long ttl])
   Sets a value in the specific cache 
   Returns true on success */
PHP_FUNCTION(apcup_set)
{
    if (APG(shared) && APG(initialized)) {
        APC_RLOCK(apcup->meta);
        {
            
            zend_ulong cache = 0L;
            char *key = NULL;
            zend_uint klen = 0L;
            zend_uint ttl = 0L;
            zval *pzval = NULL;
            
            if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "lsz|l", &cache, &key, &klen, &pzval, &ttl) != SUCCESS) {
                goto unlock;
            }
            
            if (AP_IS_CACHE(cache)) {
                apcup_cache_t* pooled = apcup_cache_find(cache TSRMLS_CC);
                if (pooled) {
                    ZVAL_BOOL(return_value, apc_cache_store(&pooled->cache, key, klen+1, pzval, ttl, 1 TSRMLS_CC));  
                }
            } else zend_error(E_WARNING, "APCu could not find the requested cache (%d)", cache);
        }
unlock:
        APC_RUNLOCK(apcup->meta);
    }
} /* }}} */

/* {{{ proto mixed apcup_get(long cache, string name) 
  Get a value from a specific cache */
PHP_FUNCTION(apcup_get)
{
    if (APG(shared) && APG(initialized)) {
        APC_RLOCK(apcup->meta);
        {
            zend_ulong cache;
            char *key;
            zend_uint klen;
            
            if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ls", &cache, &key, &klen) != SUCCESS) {
                goto unlock;
            }
            
            if (AP_IS_CACHE(cache)) {
               apcup_cache_t* pooled = apcup_cache_find(cache TSRMLS_CC);
               
               if (pooled) {
                  if (!apc_cache_fetch(&pooled->cache, key, klen+1, time(0), &return_value TSRMLS_CC)) {
                    /* not found */
                  }
               }
            } else zend_error(E_WARNING, "APCu could not find the requested cache (%d)", cache);
        }
unlock:
        APC_RUNLOCK(apcup->meta);
    }
} /* }}} */

/* {{{ proto mixed apcup_info(long cache) 
  Get info about cache */
PHP_FUNCTION(apcup_info)
{
    if (APG(shared) && APG(initialized)) {
        APC_RLOCK(apcup->meta);
        {
            zend_ulong cache;
    
            if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &cache) != SUCCESS) {
                goto unlock;
            }
            
            if (AP_IS_CACHE(cache)) {
               apcup_cache_t* pooled = apcup_cache_find(cache TSRMLS_CC);
               
               if (pooled) {
                   zval* info = apc_cache_info(&pooled->cache, 0 TSRMLS_CC);
                   
                   if (info) {
                       ZVAL_ZVAL(return_value, info, 1, 1);
                   } else ZVAL_NULL(return_value);
               } else ZVAL_NULL(return_value);
            } else zend_error(E_WARNING, "APCu could not find the requested cache (%d)", cache);
        }
unlock:
        APC_RUNLOCK(apcup->meta);
    }
} /* }}} */

/* {{{ proto void apcup_clear(long cache) 
   Clear a specific cache */
PHP_FUNCTION(apcup_clear)
{
    if (APG(shared) && APG(initialized)) {
        APC_RLOCK(apcup->meta);
        {
            zend_ulong cache;
    
            if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &cache) != SUCCESS) {
                goto unlock;
            }
            
            if (AP_IS_CACHE(cache)) {
                apcup_cache_t* pooled = apcup_cache_find(cache TSRMLS_CC);
                
                if (pooled) {
                    apc_cache_clear(&pooled->cache TSRMLS_CC);
                }
            }
        }
unlock:
        APC_RUNLOCK(apcup->meta);
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
