APCu Pooling
============

This is an extperimental extension allowing users to pool caches in userland, providing different settings for each
cache individually.

This extension requires apcu, load apcu.so first, then apcup.so

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

APCuP uses a single shared allocator, *for now*, separate from the main APCu allocator. 
It exposes enough of the APCu API for you to create and manipulate multiple caches with varying settings at runtime.

Future
======

APCuP should ideally define a way to create allocators in userland, thus allowing the user to specify an allocator for a cache.
Using a single allocator separate from APCu is a good test of the water.

Note: this (should) work(s) with APCu disabled in INI, but APCu must be loaded at runtime.
