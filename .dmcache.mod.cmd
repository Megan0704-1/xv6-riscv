savedcmd_/home/mkuo/CSE330/project6/dmcache.mod := printf '%s\n'   dm_cache.o dm_lru.o | awk '!x[$$0]++ { print("/home/mkuo/CSE330/project6/"$$0) }' > /home/mkuo/CSE330/project6/dmcache.mod
