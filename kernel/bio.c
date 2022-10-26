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

struct {
  struct spinlock lock[BUFBUCKETSIZE];
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct buf head[BUFBUCKETSIZE];
} bcache;

void
binit(void)
{
  struct buf *b;

  for (int i = 0; i < BUFBUCKETSIZE; i++)
  {
    initlock(&bcache.lock[i], "bcache");
  }
  
  
  b = bcache.buf;
  for (int i=0; i < BUFBUCKETSIZE; i++)
  {
    // Create linked list of buffers
    bcache.head[i].prev = &bcache.head[i];
    bcache.head[i].next = &bcache.head[i];

    for(int j=0; j < NBUF/BUFBUCKETSIZE; j++){
      if (b >= bcache.buf+NBUF) break;
      b->next = bcache.head[i].next;
      b->prev = &bcache.head[i];
      initsleeplock(&b->lock, "buffer");
      bcache.head[i].next->prev = b;
      bcache.head[i].next = b;
      b++;
    }
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  volatile int target = blockno % BUFBUCKETSIZE;
  struct buf *b;

  acquire(&bcache.lock[target]);

  // Is the block already cached?
  for(b = bcache.head[target].next; b != &bcache.head[target]; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.lock[target]);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  for(b = bcache.head[target].prev; b != &bcache.head[target]; b = b->prev){
    if(b->refcnt == 0) {
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      release(&bcache.lock[target]);
      acquiresleep(&b->lock);
      return b;
    }
  }
  release(&bcache.lock[target]);

  //steal
  for (int i=0; i<BUFBUCKETSIZE; i++){
    if(i==target) continue;

    acquire(&bcache.lock[i]);

    for(b = bcache.head[i].prev; b != &bcache.head[i]; b = b->prev){
      if(b->refcnt == 0) {
        //remove from origal list
        b->prev->next = b->next;
        b->next->prev = b->prev;
        release(&bcache.lock[i]);

        //add to tail of target list
        acquire(&bcache.lock[target]);
        b->next = &bcache.head[target];
        b->prev = bcache.head[target].prev;
        bcache.head[target].prev->next = b;
        bcache.head[target].prev = b;
        release(&bcache.lock[target]);

        b->dev = dev;
        b->blockno = blockno;
        b->valid = 0;
        b->refcnt = 1;

        
        acquiresleep(&b->lock);
        return b;
      }
    }
    release(&bcache.lock[i]);
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
  int target = b->blockno % BUFBUCKETSIZE;
  if(!holdingsleep(&b->lock))
    panic("brelse");

  b->refcnt--;
  releasesleep(&b->lock);

  acquire(&bcache.lock[target]);
  if (b->refcnt == 0) {
    // no one is waiting for it.
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcache.head[target].next;
    b->prev = &bcache.head[target];
    bcache.head[target].next->prev = b;
    bcache.head[target].next = b;
  }
  
  release(&bcache.lock[target]);
}

void
bpin(struct buf *b) {
  int target = b->blockno % BUFBUCKETSIZE;
  acquire(&bcache.lock[target]);
  b->refcnt++;
  release(&bcache.lock[target]);
}

void
bunpin(struct buf *b) {
  int target = b->blockno % BUFBUCKETSIZE;
  acquire(&bcache.lock[target]);
  b->refcnt--;
  release(&bcache.lock[target]);
}


