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

#include "SAPI.h"
#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_apcup.h"

ZEND_DECLARE_MODULE_GLOBALS(apcup)

/* {{{ globals */
apcup_t* apcup = {0}; /* }}} */

/* {{{ apcup_gc: run when apcups is low on memory */
void apcup_gc(apcup_t* runtime, size_t size TSRMLS_DC) {
    if (runtime && runtime->meta) {
        /* this might seem harsh ... */
        APC_LOCK(runtime->meta);
        {
            int current = 0, 
                end = runtime->meta->end,
                factor = (end > 1) ? (end - 1) : (0);
            /* run expunge on all caches created so far */
            {
                if (factor) {
                    while (current < end) {
                        if (runtime->list[current]) {
                            apc_cache_default_expunge(
                                &runtime->list[current]->cache,
                                /* this makes a guestimate,
                                    spreading the weight of gc across all caches */
                                (size / factor) TSRMLS_CC
                            );
                        } else break;
                        current++;
                    }
                }
            }
        }
        APC_UNLOCK(runtime->meta);
    }
} /* }}} */

/* {{{ implement sma */
apc_sma_api_impl(apcups, &apcup, apcup_gc); /* }}} */

/* {{{ arginfo */
ZEND_BEGIN_ARG_INFO_EX(apcup_create_arginfo, 0, 0, 1)
    ZEND_ARG_INFO(0, name)
    ZEND_ARG_INFO(0, entries_hint)
    ZEND_ARG_INFO(0, gc_ttl)
    ZEND_ARG_INFO(0, ttl)
    ZEND_ARG_INFO(0, smart)
    ZEND_ARG_INFO(0, slam_defense)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(apcup_get_arginfo, 0, 0, 2)
    ZEND_ARG_INFO(0, cache)
    ZEND_ARG_INFO(0, key)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(apcup_set_arginfo, 0, 0, 3)
    ZEND_ARG_INFO(0, cache)
    ZEND_ARG_INFO(0, key)
    ZEND_ARG_INFO(0, value)
    ZEND_ARG_INFO(0, ttl)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(apcup_info_arginfo, 0, 0, 1)
    ZEND_ARG_INFO(0, cache)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(apcup_clear_arginfo, 0, 0, 1)
    ZEND_ARG_INFO(0, cache)
ZEND_END_ARG_INFO() /* }}} */

/* {{{ apcup_functions[] */
const zend_function_entry apcup_functions[] = {
	PHP_FE(apcup_create, apcup_create_arginfo)
	PHP_FE(apcup_get,    apcup_get_arginfo)
	PHP_FE(apcup_set,    apcup_set_arginfo)
	PHP_FE(apcup_info,   apcup_info_arginfo)
	PHP_FE(apcup_clear,  apcup_clear_arginfo)
	PHP_FE_END
}; /* }}} */

/* {{{ apcup_deps [] */
const zend_module_dep apcup_deps[] = {
    ZEND_MOD_REQUIRED("apcu")
    ZEND_MOD_END
}; /* }}} */

/* {{{ apcup_module_entry
 */
zend_module_entry apcup_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
	STANDARD_MODULE_HEADER_EX,
#endif
    NULL,
    apcup_deps,
	"apcup",
	apcup_functions,
	PHP_MINIT(apcup),
	PHP_MSHUTDOWN(apcup),
	NULL,
	NULL,
	PHP_MINFO(apcup),
#if ZEND_MODULE_API_NO >= 20010901
	"0.1",
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
    /* {{{ SMA */
    STD_PHP_INI_ENTRY("apcup.shared",    "32",    PHP_INI_SYSTEM, OnUpdateLong,    shared,    zend_apcup_globals, apcup_globals)
    STD_PHP_INI_ENTRY("apcup.segments",  "1",     PHP_INI_SYSTEM, OnUpdateLong,    segments,  zend_apcup_globals, apcup_globals)
    STD_PHP_INI_ENTRY("apcup.mask",      NULL,    PHP_INI_SYSTEM, OnUpdateString,  mask,      zend_apcup_globals, apcup_globals) /* }}} */
    
    /* {{{ Caching */
    STD_PHP_INI_ENTRY("apcup.caches",    "8",     PHP_INI_SYSTEM, OnUpdateLong,    caches,    zend_apcup_globals, apcup_globals) /* }}} */
    
    /* {{{ Operation */
    STD_PHP_INI_ENTRY("apcup.cli",       "1",     PHP_INI_SYSTEM, OnUpdateBool,    cli,       zend_apcup_globals, apcup_globals) /* }}} */
PHP_INI_END()
/* }}} */

/*
 * apcup_cache_id
 * returns the id of a previously registered cache
 * assumes you hold a lock on the cache list while this occurs
 * will return 0L if not found
 */
static inline int apcup_cache_id(char* name, size_t nlength TSRMLS_DC) {
    if (apcup && apcup->meta) {
        int current = 0, end = apcup->meta->end;
        {
            if (end > 1) {
                while (current < end) {
                    if (apcup->list[current]) {
                        if (strncmp(name, apcup->list[current]->name, nlength) == SUCCESS) {
                            return apcup->list[current]->id;
                        }
                        current++;
                    } else break;
                }
            }
        }
    }
    return -1;
}

/*
 * apcup_cache_find
 * find a previously registered cache by id
 */
static inline apcup_cache_t* apcup_cache_find(zend_long id TSRMLS_DC) {
    if (apcup && apcup->meta) {
        apcup_cache_t* found = NULL;
        {
            APC_RLOCK(apcup->meta);
            if (id > 0 && 
                id < apcup->meta->end) {
                found = apcup->list[id - 1];
            }
            APC_RUNLOCK(apcup->meta);
        }
        return found;
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
                                           size_t nlength,
                                           zend_long entries_hint,
                                           zend_long gc_ttl,
                                           zend_long ttl,
                                           zend_long smart,
                                           zend_bool slam_defense TSRMLS_DC) {
    zend_bool result = 0;
    
    HANDLE_BLOCK_INTERRUPTIONS();
    
    APC_LOCK(apcup->meta);
    
    /* ensure we are within limits */
    if (apcup->meta->end < APG(caches)) {
    
        /* ensure this cache is not created twice */
        int id = apcup_cache_id(name, nlength TSRMLS_CC);
        
        /* continue if there is no result */
	    if (id == -1) {
	        
	        /* allocate new pooled cache object in shm */
	        apcup_cache_t* create = apcups.smalloc(sizeof(apcup_cache_t) TSRMLS_CC);
	        
		    if (create) {
			    /* create cache */
			    {
			        /* apc allocates using local malloc() */
			        apc_cache_t* apc = apc_cache_create(
			            &apcups,
			            0,
			            entries_hint,
			            gc_ttl, ttl,
			            smart,
			            slam_defense TSRMLS_CC
			        );
			        
			        if (apc) {
			            /* copy to shm */
			            create->cache = *apc;
			            /* free original pointer */
			            apc_efree(apc TSRMLS_CC);
			        } else goto failure;
			    }
			
			    /* set name */
			    create->name = apc_xmemcpy(
				    name, nlength, apcups.smalloc TSRMLS_CC);
			    create->nlength = nlength;
			
			    /* set id */
			    create->id = (apcup->meta->end)++;
			
			    /* register constant id for cache as user */
			    zend_register_long_constant(
			        create->name, 
			        create->nlength, 
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
            APG(caches)
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

/* {{{ php_apcup_init_globals
 */
static void php_apcup_init_globals(zend_apcup_globals *apcup_globals)
{
    apcup_globals->shared = 32;
    apcup_globals->segments = 1;
	apcup_globals->mask = NULL;
	apcup_globals->caches = 8;
	apcup_globals->cli = 1;
	
	apcup_globals->initialized = 0;
}
/* }}} */

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(apcup)
{
	ZEND_INIT_MODULE_GLOBALS(apcup, php_apcup_init_globals, NULL);
	
	REGISTER_INI_ENTRIES();
    
    if (APG(shared)) {
        /* safer */
        if (!apcups.initialized) {
        
            /* sapi check */
            if (!APG(cli)) {
            
                if (strcmp(sapi_module.name, "cli") == SUCCESS) {
                    /* stop any other 
                        contexts checking again */
                    APG(shared) = 0;
                    
                    return 1;
                }
            }
            
            /* main object initialization */
            if (!apcups.initialized) {

                /* initialize locking */
                apc_lock_init(TSRMLS_C);

                /* initialize sma */
                apcups.init(
                    APG(segments), 1024 * 1024 * APG(shared), APG(mask) TSRMLS_CC);
                
                {
                    /* initialized in shared memory */
                    apcup = (apcup_t*) apcups.smalloc(sizeof(apcup_t) TSRMLS_CC);
                    
                    if (apcup) {
                        /* calculate required size */
                        size_t required = (sizeof(apcup_cache_t*) * APG(caches)) + sizeof(apcup_meta_t);
                        
                        /* allocate shm */
                        apcup->shm = apcups.smalloc(required TSRMLS_CC);
                        
                        if (apcup->shm) {
                            /* zero shm, makes for easier debug */
                            memset(apcup->shm, 0, required);
                            
                            /* set meta at start of shm */
                            apcup->meta = (apcup_meta_t*) apcup->shm;
                            
                            if (apcup->meta) {
                                /* create a lock for safety */
                                CREATE_LOCK(&apcup->meta->lock);
                                
                                /* set next id for cache */
                                apcup->meta->end = 1;
                                
	                            /* point list at end of meta */
                                apcup->list = (apcup_cache_t**) (((char*) apcup->shm) + sizeof(apcup_meta_t));
                            
                                return SUCCESS;
                            }
                        }
                    }
                }
                
                zend_error(E_WARNING, "APCu Pooling failed to startup, try raising apcup.shared");
                
            }
        }
    }

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(apcup)
{
	UNREGISTER_INI_ENTRIES();
	
    if (apcup && apcup->meta) {
        if (apcups.initialized) {
            /* apcup shutdown */
            APC_LOCK(apcup->meta);
            {
                /* destroy caches */
                {
                    int current = 0, end = apcup->meta->end;
                    
                    if (end > 1) {
                        while (current < end) {
                            if (apcup->list[current]) {
                                DESTROY_LOCK(&apcup->list[current]->cache.header->lock);
                                
                                if (apcup->list[current]->name) {
                                    apcups.sfree(apcup->list[current]->name TSRMLS_CC);
                                }
                            } else break;
                            
                            current++;
                        }
                    }
                }
            }
            /* we are out */
            APC_UNLOCK(apcup->meta);
            
            /* free meta */
            apcups.sfree(apcup->meta TSRMLS_CC);
            
            /* free shm */
            apcups.sfree(apcup->shm TSRMLS_CC);
            
            /* free apcup */
            apcups.sfree(apcup TSRMLS_CC);
            
            /* cleanup sma */
            apcups.cleanup(TSRMLS_C);
            
            /* be sure */
            apcup = NULL;
        }
    }

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
    if (apcups.initialized) {
        zend_string *name = NULL;
        zend_long entries_hint = 1024;
        zend_long gc_ttl = 0L;
        zend_long ttl = 0L;
        zend_long smart = 0L;
        zend_bool slam_defense = 1;
        
        if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "S|llllb", &name, &entries_hint, &gc_ttl, &ttl, &smart, &slam_defense) != SUCCESS) {
            return;
        }
        
        RETURN_BOOL(apcup_create_cache(ZSTR_VAL(name), ZSTR_LEN(name), entries_hint, gc_ttl, ttl, smart, slam_defense TSRMLS_CC));
    }
} /* }}} */

/* {{{ proto boolean apcup_set(long cache, string key, mixed value, [, long ttl])
   Sets a value in the specific cache 
   Returns true on success */
PHP_FUNCTION(apcup_set)
{
    if (apcups.initialized) {
        zend_long cache = 0L;
        zend_string *key = NULL;
        zend_long ttl = 0L;
        zval *pzval = NULL;

        if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "lSz|l", &cache, &key, &pzval, &ttl) != SUCCESS) {
            return;
        }

        {
            apcup_cache_t* pooled = apcup_cache_find(cache TSRMLS_CC);
            if (pooled) {
                ZVAL_BOOL(return_value, apc_cache_store(&pooled->cache, key, pzval, ttl, 1 TSRMLS_CC));  
            } else zend_error(E_WARNING, "APCu could not find the requested cache (%d)", cache);
        }
    }
} /* }}} */

/* {{{ proto mixed apcup_get(long cache, string key) 
  Get a value from a specific cache */
PHP_FUNCTION(apcup_get)
{
    if (apcups.initialized) {
        zend_long cache;
        zend_string *key = NULL;
        
        if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "lS", &cache, &key) != SUCCESS) {
            return;
        }
        
        
        {
            apcup_cache_t* pooled = apcup_cache_find(cache TSRMLS_CC);
               
            if (pooled) {
                if (!apc_cache_fetch(&pooled->cache, key, time(0), &return_value TSRMLS_CC)) {
                    /* not found */
                }
            } else zend_error(E_WARNING, "APCu could not find the requested cache (%d)", cache);
        }
    }
} /* }}} */

/* {{{ proto mixed apcup_info(long cache) 
  Get info about cache */
PHP_FUNCTION(apcup_info)
{
    if (apcups.initialized) {
        zend_long cache = 0L;

        if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &cache) != SUCCESS) {
            return;
        }
        
        {
            switch (cache) {
                case 0: {
                    /* TODO XXX implement sma info */
                    ZVAL_NULL(return_value);
                } break;
                
                default: {
                    apcup_cache_t* pooled = apcup_cache_find(cache TSRMLS_CC);
                       
                    if (pooled) {
                       zval info = apc_cache_info(&pooled->cache, 0 TSRMLS_CC);

                       if (Z_TYPE(info) != IS_UNDEF) {
                           ZVAL_ZVAL(return_value, &info, 1, 1);
                       } else ZVAL_NULL(return_value);
                    } else zend_error(E_WARNING, "APCu could not find the requested cache (%d)", cache);
                }
            }   
        }
    }
} /* }}} */

/* {{{ proto void apcup_clear(long cache) 
   Clear a specific cache */
PHP_FUNCTION(apcup_clear)
{
    if (apcups.initialized) {
        zend_long cache = 0L;

        if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &cache) != SUCCESS) {
            return;
        }
        
        
        {
            apcup_cache_t* pooled = apcup_cache_find(cache TSRMLS_CC);

            if (pooled) {
                apc_cache_clear(&pooled->cache TSRMLS_CC);
            } else zend_error(E_WARNING, "APCu could not find the requested cache (%d)", cache);
        }
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
