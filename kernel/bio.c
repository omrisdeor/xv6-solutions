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

#define BUCKET_COUNT 13

struct head {
  struct spinlock lock;
  struct buf head;
};


struct {
  struct spinlock lock;
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct head heads[BUCKET_COUNT];
} bcache;

void
binit(void)
{
  struct buf *b;

  initlock(&bcache.lock, "bcache");

  for (int i = 0; i < BUCKET_COUNT; i++)
  {
    initlock(&bcache.heads[i].lock, "bcache");
    bcache.heads[i].head.prev = &bcache.heads[i].head;
    bcache.heads[i].head.next = &bcache.heads[i].head;
  }

  // Create linked list of buffers
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    b->next = bcache.heads[(b - bcache.buf) % BUCKET_COUNT].head.next;
    b->prev = &bcache.heads[(b - bcache.buf) % BUCKET_COUNT].head;
    initsleeplock(&b->lock, "buffer");
    bcache.heads[(b - bcache.buf) % BUCKET_COUNT].head.next->prev = b;
    bcache.heads[(b - bcache.buf) % BUCKET_COUNT].head.next = b;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;

  int hash = blockno % BUCKET_COUNT;

  acquire(&bcache.heads[hash].lock);
  for(b = bcache.heads[hash].head.next; b != &bcache.heads[hash].head; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.heads[hash].lock);
      acquiresleep(&b->lock);
      return b;
    }
  }

  acquire(&bcache.lock);

  for (int full_buck = hash; full_buck < BUCKET_COUNT + hash; full_buck++)
  {
    int buck = full_buck % BUCKET_COUNT;
    if (buck != hash)
    {
      acquire(&bcache.heads[buck].lock);
    }
    for (b = bcache.heads[buck].head.next; b != &bcache.heads[buck].head; b = b->next)
    {
      if (b->refcnt == 0)
      {
        b->dev = dev;
        b->blockno = blockno;
        b->valid = 0;
        b->refcnt = 1;
        if (buck != hash)
        {
          b->next->prev = b->prev;
          b->prev->next = b->next;
          b->next = bcache.heads[hash].head.next;
          b->prev = &bcache.heads[hash].head;
          bcache.heads[hash].head.next->prev = b;
          bcache.heads[hash].head.next = b;
          release(&bcache.heads[buck].lock);
        }
        release(&bcache.lock);
        release(&bcache.heads[hash].lock);
        acquiresleep(&b->lock);
        return b;
      }
    }
    if (buck != hash)
    {
      release(&bcache.heads[buck].lock);
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
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  int hash = b->blockno % BUCKET_COUNT;

  acquire(&bcache.heads[hash].lock);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcache.heads[hash].head.next;
    b->prev = &bcache.heads[hash].head;
    bcache.heads[hash].head.next->prev = b;
    bcache.heads[hash].head.next = b;
  }
  
  release(&bcache.heads[hash].lock);
}

void
bpin(struct buf *b) {
  acquire(&bcache.lock);
  b->refcnt++;
  release(&bcache.lock);
}

void
bunpin(struct buf *b) {
  acquire(&bcache.lock);
  b->refcnt--;
  release(&bcache.lock);
}


