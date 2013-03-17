APCu Pooling
============

This extension allows users to pool caches in userland, providing different settings for each cache individually.

This extension requires apcu, load apcu.so first, then apcup.so

Should you decide to use pools in production, it is a good idea to disable the main APCu cache with apc.enabled = 0 in INI.

Quick How To
============

You provide names for your caches, which are then registered as constants using the given name.

Here is the prototype for apcup_create():
    long apcup_create(string name [, long entries_hint = 1024, [ long gc_ttl = 0, [ long ttl = 0, [ long smart = 0, [ bool slam_defense = true]]]]])
    
Example:
    apcup_create("APCUP_DEFAULT", 2048, 3060);
    
    boolean apcup_set(long cache, string name, mixed value, [, long ttl])
    Example:
        ```php
        apcup_set(APCUP_DEFAULT, "key", $value);
        ```

    mixed apcup_get(long cache, string name)
    Example:
        ```php
        apcup_get(APCUP_DEFAULT, "key");
        ```

    mixed apcup_clear(long cache)
    Example:
        ```php
        apcup_clear(APCUP_DEFAULT);
        ```

    mixed apcup_info(long cache)
    Example:
        ```php
        apcup_info(APCUP_DEFAULT);
        ```

INI
===

apcup.shared: 
    size of each segment (MB)
    Default: (32)
    
apcup.segments: 
    number of segments to use
    Default: (1)
    
apcup.mask:
    mmap file mask
    Default: (null)
    
apcup.caches: 
    the maximum number of caches that can be created
    Default: (8)


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

APCu must be loaded, but should you use pools, you should use apc.enable = 0 in your configuration settings, this will stop APCu from setting
up the default user cache and allow you to pool for threads.
