#include <linux/dm-io.h>
#include "dm_cache.h"
#include "dm.h"

#define DM_MSG_PREFIX "cache"
#define DMC_PREFIX "dm-cache: "

#define DEFAULT_CACHE_SIZE  65536
#define DEFAULT_BLOCK_SIZE  8

#define cache_stats_inc(x)       atomic64_inc(&(dmc->stats).x)
#define cache_stats_get(x)       (uint64_t)atomic64_read((&(dmc->stats).x))

struct cache_stats {
    atomic64_t reads;                /* Number of reads */
    atomic64_t writes;               /* Number of writes */
    atomic64_t cache_hits;           /* Number of cache hits */
    atomic64_t cache_misses;         /* Number of cache Misses */
};

/* dm_cache context structure */
struct dm_cache_c {
    char src_dev_name[100];
    char cache_dev_name[100];

    struct dm_dev *src_dev;     	    /* Source device */
    struct dm_dev *cache_dev;   	    /* Cache device */
    spinlock_t cache_lock;              /* Lock to protect the radix tree */
    struct radix_tree_root *cache;  	/* Radix Tree for cache blocks */
    sector_t cache_size;       		    /* Cache size in sectors excluding OPS */
    unsigned int block_size;    	    /* Cache block size */
    unsigned int block_shift;   	    /* Cache block size in bits */
    unsigned int block_mask;    	    /* Cache block mask */

    struct dm_io_client *io_client;   	/* Client memory pool*/
    struct cache_stats stats;           /* cache_c stats */

    /* LRU List */
    struct list_head *lru;
    struct bio *bio;
}

// dm_cache c holds config and state of the caching system
static struct dm_cache_c *dm_cache;

static void do_write_endio_cb(struct bio *bio)
{
    bio_free_pages(bio);
    bio_put(bio);
}

void
do_read(struct block_device *blkdev, sector_t addr)
{
    struct bio *bio = dm_cache->bio;
    bio->bi_iter.bi_sector = addr;
    bio->bi_opf = REQ_OP_READ;
    bio_set_dev(bio, blkdev);
    submit_bio_noacct(bio);
}

void
do_write(struct block_device *blkdev, sector_t addr)
{
    struct bio *bio = dm_cache->bio;
    struct bio *local_bio;
    struct page *page;

    local_bio = bio_alloc(blkdev, 1, REQ_OP_READ, GFP_NOIO);
    page = alloc_page(GFP_KERNEL);

    local_bio->bi_iter.bi_sector = addr;
    local_bio->bi_opf = REQ_OP_WRITE;
    bio_set_dev(local_bio, blkdev);

    if (!bio_add_page(local_bio, page, 4096, 0))
        BUG();
    bio_copy_data(local_bio, bio);

    local_bio->bi_end_io = do_write_endio_cb;
    submit_bio_noacct(local_bio);
}

static void dmcache_status(struct dm_target* ti, status_type_t type,
                            unsigned status_flags, char* result, unsigned maxlen)
{
    struct dm_cache_c *dmc = (struct dm_cache_c*)ti->private;
    switch (type) {
        case STATUSTYPE_INFO:
            DMINFO("+++++++++++++++++ Cache: Statistics ++++++++++++++++++");
            DMINFO("- Reads:%llu", cache_stats_get(reads));
            DMINFO("- Writes:%llu", cache_stats_get(writes));
            DMINFO("- Cache Hits:%llu", cache_stats_get(cache_hits));
            DMINFO("- Cache Miss:%llu", cache_stats_get(cache_misses));
            DMINFO("++++++++++++++++++++++++++++++++++++++++++++++++++++++++++");
            break;
        default:
            break;
    }
}

// Updates a blk on src device after a cache blk is replaced with a new blk.
static int update_mapping(struct dm_cache_c *dmc, sector_t block, struct cacheblock *cache)
{
    cache->src_block_addr = block >> dmc->block_shift;
    spin_lock(&dmc->cache_lock);
    radix_tree_insert(dmc->cache, block >> dmc->block_shift, (void *) cache);
    spin_unlock(&dmc->cache_lock);
    return 1;
}

// Calls `my_cache_hit` to handle hits
static int cache_hit(struct dm_cache_c *dmc, struct bio* bio, struct cacheblock *cache)
{
    cache_stats_inc(cache_hits); // atomic incr dmc cache hit counts
    my_cache_hit(dmc->cache_dev->bdev, cache, dmc->lru);
    return DM_MAPIO_SUBMITTED;
}

// Calls `my_cache_miss` to handle misses
static int cache_miss(struct dm_cache_c *dmc, struct bio* bio)
{
    struct cacheblock* cache;
    cache_stats_inc(cache_misses);

    const unsigned int offset = (unsigned int)(bio->bi_iter.bi_sector & dmc->block_mask);
    const sector_t request_block = bio->bi_iter.bi_sector - offset;

    cache = my_cache_miss(dmc->src_dev->bdev, dmc->cache_dev->bdev, request_block, dmc->lru);
    update_mapping(dmc, bio->bi_iter.bi_sector, cache);
	return DM_MAPIO_SUBMITTED;
}

// Checks cache map for blk number, if exist, cache hit, o.w. cache miss.
static struct cacheblock *cache_lookup(struct dm_cache_c *dmc, const sector_t blk_no)
{
    struct cacheblock *ret = NULL;
    spin_lock(&dmc->cache_lock);
    ret = radix_tree_lookup(dmc->cache, blk_no);
    spin_unlock(&dmc->cache_lock);
    return ret;
}

// `The cache map handles the IO request: call upon dm-cache receives IO request`
static int dmcache_map(struct dm_target* ti, struct bio* bio)
{
    struct dm_cache_c *dmc = (struct dm_cache_c*)ti->private;
    struct cacheblock *cache;
    sector_t request_block, offset;

    offset 	       = bio->bi_iter.bi_sector & dmc->block_mask;
    request_block  = bio->bi_iter.bi_sector - offset;
    dmc->bio       = bio;

    if (bio_data_dir(bio) == READ)
        cache_stats_inc(reads);
    else {
        bio_set_dev(bio, dmc->src_dev->bdev);
        cache_stats_inc(writes);
        return DM_MAPIO_REMAPPED;
    }

    cache = cache_lookup(dmc, request_block >> dmc->block_shift);
    if (cache != NULL)  {
        return cache_hit(dmc, bio, cache);
    }
    else {
        return cache_miss(dmc, bio);
    }
}

static int dmcache_message(struct dm_target* ti, unsigned argc, 
                            char** argv, char* result, unsigned maxlen)
{
	return 1;
}

//  `Cache destructor`
// Params
// - target cache device.
static void dmcache_dtr(struct dm_target* ti)
{
    struct dm_cache_c *dmc = (struct dm_cache_c*)ti->private;
    free_lru(dmc->lru);

    dm_io_client_destroy(dmc->io_client);
    dm_put_device(ti,dmc->cache_dev);
    dm_put_device(ti,dmc->src_dev);
    kvfree((void *)dmc->cache);
    kvfree(dmc);
}

//  `Cache constructor: configures the source & target device`
// Params
// - ti <struct dm_target*> = device mapper target device
// - argc <ui> = # of args
// - argv <char**> = args array, expecting a source device and a target cache
static int dmcache_ctr(struct dm_target* ti, unsigned int argc, char* *argv)
{
    struct dm_cache_c *dmc;
    sector_t data_size, dev_size;
    unsigned long long cache_size;
    int ret = -EINVAL;

    DMINFO("Name: %s", dm_table_device_name(ti->table)); // log

    // Read all the command line parameters passed to the dmcache device driver
    if (argc < 2) {
        ti->error = "dm-cache: Need at least 2 arguments (src dev and cache dev)";
        goto bad;
    }

    // allocate cache context
    dm_cache = dmc = (struct dm_cache_c *)kvmalloc(sizeof(*dmc), GFP_KERNEL);
    if (dmc == NULL) {
        ti->error   = "dm-cache: Failed to allocate cache context";
        ret         = ENOMEM;
        goto bad;
    }

    // Adding source device: arg[0]: path to source device
    ret = dm_get_device(ti, argv[0], dm_table_get_mode(ti->table), &dmc->src_dev);
    if (ret) {
        ti->error = "dm-cache: Source device lookup failed";
        goto bad1;
    }

    // Adding cache device: arg[1]: path to cache device
    ret = dm_get_device(ti, argv[1], dm_table_get_mode(ti->table), &dmc->cache_dev);
    if (ret) {
        ti->error = "dm-cache: Cache device lookup failed";
        goto bad1;
    }

    strcpy(dmc->src_dev_name, argv[0]);
    strcpy(dmc->cache_dev_name, argv[1]);

    // create IO client for async IO ops
    dmc->io_client = dm_io_client_create();

    if (IS_ERR(dmc->io_client)) {
        ret         = PTR_ERR(dmc->io_client);
        ti->error   = "Failed to create io client\n";
        goto bad2;
    }

    // Read: arg[2]: cache block size (in sectors)
    if (argc >= 3) {
        if (sscanf(argv[2], "%u", &dmc->block_size) != 1) {
            ti->error   = "dm-cache: Invalid block size";
            ret         = -EINVAL;
            goto bad3;
        }
        if (!dmc->block_size || (dmc->block_size & (dmc->block_size - 1))) {
            ti->error   = "dm-cache: Invalid block size";
            ret         = -EINVAL;
            goto bad3;
        }
    } else {
        dmc->block_size = DEFAULT_BLOCK_SIZE;
    }

    // arg[3]: cache size (in blocks)
    if (argc >= 4) {
        if (sscanf(argv[3], "%llu", &cache_size) != 1) {
            ti->error   = "dm-cache: Invalid cache size";
            ret         = -EINVAL;
            goto bad3;
        }
        dmc->cache_size = (sector_t) cache_size;
    } else {
        dmc->cache_size = DEFAULT_CACHE_SIZE;
    }

    dmc->block_shift 	= ffs(dmc->block_size) - 1;
    dmc->block_mask 	= dmc->block_size - 1;

    dev_size  = bdev_nr_sectors(dmc->cache_dev->bdev);
    data_size = dmc->cache_size * dmc->block_size;

    if (data_size > dev_size) {
        DMERR("Requested cache size exeeds the cache device's capacity(%llu > %llu)",
                (unsigned long long) data_size, (unsigned long long) dev_size);
        ti->error = "dm-cache: Invalid cache size";
        ret       = -EINVAL;
        goto bad3;
    }

    spin_lock_init(&dmc->cache_lock);

    // Initialize the radix trees
    dmc->cache = (struct radix_tree_root *)kvmalloc(sizeof(*dmc->cache), GFP_KERNEL);
    if (!dmc->cache) {
        ti->error   = "Unable to allocate memory";
        ret         = -ENOMEM;
        goto bad3;
    }
    INIT_RADIX_TREE(dmc->cache, GFP_NOWAIT);

    // Initialize the LRU
    dmc->lru = (struct list_head *)kvmalloc(sizeof(*dmc->lru), GFP_KERNEL);
    init_lru(dmc->lru, dmc->cache_size);

    ti->max_io_len  = dmc->block_size;

    // stores cache content in private field
    ti->private     = dmc;

    DMINFO("------------ Cache Configuration ------------");
    DMINFO("- Source Device:%s", dmc->src_dev_name);
    DMINFO("- Cache Device:%s", dmc->cache_dev_name);
    DMINFO("- Block Size: %d", dmc->block_size);
    DMINFO("----------------------------------------------");

	return 0;

bad3:
    dm_io_client_destroy(dmc->io_client);
bad2:
    dm_put_device(ti,dmc->cache_dev);
    dm_put_device(ti,dmc->src_dev);
bad1:
    kfree(dmc);
bad:
    return ret;
}


static struct target_type cache_target = {
    .name     = "cache",
    .version  = {1, 0, 1},
    .module   = THIS_MODULE,
    .ctr      = dmcache_ctr,
    .dtr      = dmcache_dtr,
    .map      = dmcache_map,
    .status   = dmcache_status,
    .message  = dmcache_message,
};


static int __init dmcache_init(void)
{
    int ret;

    ret = dm_register_target(&cache_target);
    if (ret < 0) {
        DMERR("cache: register failed %d", ret);
    }

    DMINFO("Device Mapper Target: dmcache registered successfully!!");
	return ret;
}

static void __exit dmcache_exit(void)
{
    dm_unregister_target(&cache_target);
    DMINFO("Device Mapper Target: dmcache unregistered successfully!!");
}

module_init(dmcache_init);
module_exit(dmcache_exit);

MODULE_DESCRIPTION(DM_NAME " cache target");
MODULE_AUTHOR("Kritshekhar Jha");
MODULE_LICENSE("GPL");
