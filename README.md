APCu Pooling
============

This extension allows users to pool caches in userland, providing different settings for each cache individually.

This extension requires apcu, load apcu.so first, then apcup.so

Should you decide to use pools in production, it is a good idea to disable the main APCu cache with apc.enabled = 0 in INI.

API
===

```php
/**
* Will create or register a cache in the current context
* 
* @param $name           the constant name of this cache
* @param $entries_hint   the number of entries expected
* @param $gc_ttl         the ttl for items on gc list
* @param $ttl            the ttl for items in cache
* @param $smart          the smart setting for gc time
* @param $slam_defense   enable/disable slam defense
**/
apcup_create($name, $entries_hint = 1024, $gc_ttl = 0, $ttl = 0, $smart = 0, $slam_defense = true);

/**
* Will set a value in a specific cache
* 
* @param $cache          a constant cache id
* @param $key            the name of the entry
* @param $value          the data for the entry
* @param $ttl            the ttl for the entry
**/
apcup_set($cache, $key, $value, $ttl = 0);


/**
* Will get a value from a specific cache
* 
* @param $cache          a constant cache id
* @param $key            the name of the entry
**/
apcup_get($cache, $key);

/**
* Will clear a specific cache
* 
* @param $cache          a constant cache id
**/
apcup_clear($cache);

/**
* Will return detailed information concerning a cache
* 
* @param $cache          a constant cache id
**/
apcup_info($cache);
```

INI
===

 * apcup.shared:
    _size of each segment (MB)_
    *Default*: (32)
    
 * apcup.segments: 
    _number of segments to use_
    *Default*: (1)
    
 * apcup.mask:
    _mmap file mask_
    *Default*: (null)
    
 * apcup.caches: 
    _the maximum number of caches that can be created_
    *Default*: (8)

How
===

APCuP uses a single shared allocator provided by the APCu API. It allows the user to create caches and manipulate them in any context at runtime.

Future
======

Hopefully, one day APCu(P) will have the ability to create multiple allocators to reduce lock contention on any pooled caches.

pthreads
========

Stand on the shoulders of giants, whenever you can ...

APCu has good memory management and garbage collection, at sometime in the future, APCu backed storage in pthreads will at least be an option.
In the interim, this extension can be used to great effect in order to manage ( in a far superior way ) the shared memory among threads.

APCu must be loaded, but should be disabled, this will stop APCu from setting up the default user cache and allow you to pool for threads.

Follows an example of using APCu Pooling in pthreads:

```php
/* create the cache */
apcup_create("APCUP_TEST");

class T extends Thread {
    public function run (){
        /* no need to create, though you can ... */
        var_dump(
            apcup_get(APCUP_TEST, "global"));
    }
}

/* some data from somewhere */
$my = array(
    "Hello", "World"
);

/* set the value in the cache for the thread to come ... */
apcup_set(APCUP_TEST, "global", $my);

/* go */
$t = new T();
$t->start();
```
