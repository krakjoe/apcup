APCu Pooling
============

This is an extperimental extension allowing users to pool caches in userland, providing different settings for each
cache individually.

The only ini setting introduced is apcup.shared to set amount of shared memory.

This extension requires apcu, load apcu.so first, then apcup.so

Quick How To
============

You provide names for your caches, which are then registered as constants and returned, throughout execution you reference the constant id
so we can read list unlocked, for very fast access to multiple caches concurrently

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

This is an experiment, that is all ...
    
    
    
