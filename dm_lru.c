#include <linux/list.h>
#include "dm_cache.h"

/**
 * my_cache_hit - update LRU & read block from cache
 * @cache_blkdev: cache dev ptr
 * @block: ptr to the hit cache block
 * @lru_head: LRU list head
 */
void
my_cache_hit(struct block_device *cache_blkdev, struct cacheblock *block,
        struct list_head *lru_head)
{
    /** list_move_tail(list, head): 
     * @list: entry to remove
     * @head: list_add_tail 
     */ // Update LRU: moves hit block to LRU list tail -> MRU
    list_move_tail(&block->list, lru_head);

    // Read the block from cache
    do_read(cache_blkdev, block->cache_block_addr);
}

/**
 * my_cache_miss - reads block from src dev, updates LRU and returns a cache block struct
 * @src_blkdev: source block dev ptr
 * @cache_blkdev: cache block dev ptr
 * @src_blkaddr: source block addr
 * @lru_head: LRU list head
 * Note: it returns an updated cache block
 */
struct cacheblock*
my_cache_miss(struct block_device *src_blkdev, struct block_device *cache_blkdev,
        sector_t src_blkaddr, struct list_head *lru_head)
{
    struct cacheblock *block; /* update this as per the steps in the project document*/ 
    // list_first_entry gets the first ele from @list, as lru_head ptr points to the tail of the list
    block = list_first_entry(lru_head, struct cacheblock, list);
    
    // moves to MRU
    list_move_tail(&block->list, lru_head);

    // read blk from src devices
    do_read(src_blkdev, src_blkaddr);

    // write to cache
    do_write(cache_blkdev, block->cache_block_addr);

    // updates LRU: 
    return block;
}

/**
 * list_add - add a new entry
 * @new: new entry to be added
 * @head: list head to add it after
 *
 * new->prev = head
 * new->next = head->next
 * head->next = new
 * new->next->prev = new
 */
void
init_lru(struct list_head *lru_head, unsigned int num_blocks)
{
    // Initialize the head of LRU
    INIT_LIST_HEAD(lru_head);

    // Initialize nodes of the LRU list (node is of type cache block struct)
    for(int i=0; i<num_blocks; ++i) {
        struct cacheblock *cbp = (struct cacheblock *)kvmalloc(sizeof(struct cacheblock), GFP_KERNEL);

        cbp->src_block_addr = 0;
        cbp->cache_block_addr = i >> BLOCK_SHIFT;
        list_add(&cbp->list /* new */, lru_head /* head */);
    }
}

void
free_lru(struct list_head *lru_head)
{
    struct cacheblock *blk, *safety;

    list_for_each_entry_safe(blk, safety, lru_head, list) {
        list_del(&blk->list);
        kvfree(blk);
    }

    kvfree(lru_head);
}
