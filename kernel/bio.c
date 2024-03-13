// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.

#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

#define HASHSIZE 5

struct {
  struct {
    struct sleeplock lock;
    struct buf buf[6];

    // Linked list of all buffers, through prev/next.
    // Sorted by how recently the buffer was used.
    // lru.next is most recent, lru.prev is least.
    struct buf lru;

    // Entries are in use by processes
    struct buf inuse;
  } bcache[HASHSIZE];
} bcachetable;

void
binit(void)
{
  struct buf *b;

  for (int i = 0; i < HASHSIZE; i++) {
    initsleeplock(&bcachetable.bcache[i].lock, "bcache");
    // Create linked list of buffers
    bcachetable.bcache[i].lru.prev = &bcachetable.bcache[i].lru;
    bcachetable.bcache[i].lru.next = &bcachetable.bcache[i].lru;
    bcachetable.bcache[i].inuse.prev = &bcachetable.bcache[i].inuse;
    bcachetable.bcache[i].inuse.next = &bcachetable.bcache[i].inuse;
    for (b = bcachetable.bcache[i].buf; b < bcachetable.bcache[i].buf + 6;
         b++) {
      b->refcnt = 0;
      b->next = bcachetable.bcache[i].lru.next;
      b->prev = &bcachetable.bcache[i].lru;
      initsleeplock(&b->lock, "buffer");
      bcachetable.bcache[i].lru.next->prev = b;
      bcachetable.bcache[i].lru.next = b;
    }
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  int idx = blockno % HASHSIZE;

  // Is the block already cached?
  acquiresleep(&bcachetable.bcache[idx].lock);
  for (b = bcachetable.bcache[idx].inuse.next;
       b != &bcachetable.bcache[idx].inuse; b = b->next) {
    if (b->dev == dev && b->blockno == blockno) {
      if (b->refcnt == 0)
        panic("bget"); 
      b->refcnt++;
      acquiresleep(&b->lock);
      releasesleep(&bcachetable.bcache[idx].lock);
      return b;
    }
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  // if (bcachetable.bcache[idx].lru.prev == &bcachetable.bcache[idx].lru) {
  //   brelse(bcachetable.bcache[idx].inuse.prev);
  // }
  for (b = bcachetable.bcache[idx].lru.prev; b != &bcachetable.bcache[idx].lru;
       b = b->prev) {
    if (b->refcnt == 0) {
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;

      b->prev->next = b->next;
      b->next->prev = b->prev;

      b->next = bcachetable.bcache[idx].inuse.next;
      b->prev = &bcachetable.bcache[idx].inuse;
      b->next->prev = b;
      b->prev->next = b;
      releasesleep(&bcachetable.bcache[idx].lock);
      acquiresleep(&b->lock);
      return b;
    }
  }
  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the lru of the most-recently-used list.
void
brelse(struct buf *b)
{
  if (!holdingsleep(&b->lock))
    panic("brelse");
  if (b->refcnt <= 0)
    panic("brelse");

  releasesleep(&b->lock);

  int idx = b->blockno % HASHSIZE;
  acquiresleep(&bcachetable.bcache[idx].lock);
  b->refcnt--;
  if (b->refcnt <= 0) {
    b->refcnt = 0;
    // no one is waiting for it.
    b->next->prev = b->prev;
    b->prev->next = b->next;

    b->next = bcachetable.bcache[idx].lru.next;
    b->prev = &bcachetable.bcache[idx].lru;
    b->next->prev = b;
    b->prev->next = b;
  }

  releasesleep(&bcachetable.bcache[idx].lock);
}

void
bpin(struct buf *b) {
  b->refcnt++;
}

void
bunpin(struct buf *b) {
  b->refcnt--;
}
