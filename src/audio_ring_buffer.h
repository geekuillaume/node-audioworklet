/*
 * Copyright © 2016 Mozilla Foundation
 *
 * This program is made available under an ISC-style license.  See the
 * accompanying file LICENSE for details.
 */

// Taken from cubeb_ringbuffer.h and modified to handle audio buffer with a timestamp

#pragma once

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <memory>
#include <thread>
#include <assert.h>
#include <math.h>
#include "debug.h"

// can push a chunk with a minimum of 480 bytes
#define CHUNK_SIZE_BYTES 480

typedef struct AudioChunk {
  uint64_t timestamp;
  uint32_t frames;
  uint8_t chunk[CHUNK_SIZE_BYTES];
  // number of frames already consumed
  uint32_t consumed;
} AudioChunk;

class AudioRingBuffer
{
public:
  AudioRingBuffer(int framesCapacity, int bytesPerFrame)
    /* One more element to distinguish from empty and full buffer. */
    : capacity_(((framesCapacity * bytesPerFrame) / CHUNK_SIZE_BYTES) + 1),
    bytesPerFrame(bytesPerFrame)
  {
    assert(storage_capacity() <
           std::numeric_limits<int>::max() / 2 &&
           "buffer too large for the type of index used.");
    assert(capacity_ > 0);

    data_.reset(new AudioChunk[storage_capacity()]);
    /* If this queue is using atomics, initializing those members as the last
     * action in the constructor acts as a full barrier, and allow capacity() to
     * be thread-safe. */
    write_index_ = 0;
    read_index_ = 0;
  }

  // Enqueue a continous audio chunk from a timestamp
  // Audio chunks needs to be queued in chronological order, no overlay is permitted, holes are permitted
  int enqueueAudioBuffer(uint64_t timestamp, uint8_t *buffer, uint32_t framesCount) {
    #ifndef NDEBUG
        assert_correct_thread(producer_id);
    #endif

    if (framesCount == 0 || buffer == NULL) {
      return 0;
    }

    int rd_idx = read_index_.load(std::memory_order::memory_order_relaxed);
    int wr_idx = write_index_.load(std::memory_order::memory_order_relaxed);

    if (full_internal(rd_idx, wr_idx)) {
      return 0;
    }

    int writtenFrames = 0;
    int chunkToWrite =
      std::min((uint32_t)available_write_internal(rd_idx, wr_idx), (uint32_t)ceil((float)(framesCount * bytesPerFrame) / CHUNK_SIZE_BYTES));

    for (int i = 0; i < chunkToWrite; i++) {
      AudioChunk *chunk = &data_.get()[increment_index(wr_idx, i)];
      chunk->timestamp = timestamp + (i * (CHUNK_SIZE_BYTES / bytesPerFrame));
      chunk->frames = std::min(framesCount, (uint32_t)CHUNK_SIZE_BYTES / bytesPerFrame);
      chunk->consumed = 0;
      memcpy(chunk->chunk, buffer + (i * CHUNK_SIZE_BYTES), chunk->frames * bytesPerFrame);
      // debug_print("Pushed chunk at timestamp %d of size %d, queue index %d\n", chunk->timestamp, chunk->frames, increment_index(wr_idx, i));
      framesCount -= chunk->frames;
      writtenFrames += chunk->frames;
    }

    write_index_.store(increment_index(wr_idx, chunkToWrite), std::memory_order::memory_order_release);
    return writtenFrames;
  }

  int dequeueAudioBuffer(uint64_t timestamp, uint8_t *dest, uint32_t frames) {
    #ifndef NDEBUG
      assert_correct_thread(consumer_id);
    #endif

    int wr_idx = write_index_.load(std::memory_order::memory_order_acquire);
    int rd_idx = read_index_.load(std::memory_order::memory_order_relaxed);

    int filledFrames = 0;
    int chunkIndex = 0;
    while (frames > 0 && !empty_internal(increment_index(rd_idx, chunkIndex), wr_idx)) {
      AudioChunk *chunk = &data_.get()[increment_index(rd_idx, chunkIndex)];
      if (chunk->timestamp + (chunk->frames - chunk->consumed) <= timestamp) {
        // chunk end timestamp is older than requested timestamp, ignore chunk
        chunkIndex++;
        continue;
      }
      if (chunk->timestamp < timestamp) {
        // chunk timestamp is before requested timestamp but chunk end is after requested timestamp, consume unused frames
        int framesToConsume = std::min(timestamp - chunk->timestamp, (uint64_t)(chunk->frames - chunk->consumed));
        assert(framesToConsume + chunk->consumed < chunk->frames);
        chunk->consumed += framesToConsume;
        chunk->timestamp += framesToConsume;
        continue;
      }
      if (chunk->timestamp > timestamp) {
        // chunk timestamp is in the future, fill dest buffer with 0 until chunk is valid
        int framesToFill = std::min(frames, (uint32_t)(chunk->timestamp - timestamp));
        memset(dest + (filledFrames * bytesPerFrame), 0, framesToFill * bytesPerFrame);

        timestamp += framesToFill;
        frames -= framesToFill;
        filledFrames += framesToFill;
        continue;
      }
      assert(chunk->timestamp == timestamp);
      assert(chunk->consumed < chunk->frames);

      int framesToRead = std::min(chunk->frames - chunk->consumed, frames);
      memcpy(dest + (filledFrames * bytesPerFrame), chunk->chunk + (chunk->consumed * bytesPerFrame), framesToRead * bytesPerFrame);

      chunk->consumed += framesToRead;
      chunk->timestamp += framesToRead;

      timestamp += framesToRead;
      frames -= framesToRead;
      filledFrames += framesToRead;

      if (chunk->consumed == chunk->frames) {
        chunkIndex++;
      }
    }

    if (frames > 0) {
      memset(dest + (filledFrames * bytesPerFrame), 0, frames * bytesPerFrame);
    }

    read_index_.store(increment_index(rd_idx, chunkIndex), std::memory_order::memory_order_relaxed);
    return filledFrames;
  }

  /**
   * Get the number of available element for consuming.
   *
   * Only safely called on the consumer thread.
   *
   * @return The number of available elements for reading.
   */
  int available_read() const
  {
    #ifndef NDEBUG
      assert_correct_thread(consumer_id);
    #endif
    int wr_idx = write_index_.load(std::memory_order::memory_order_acquire);
    int rd_idx = read_index_.load(std::memory_order::memory_order_relaxed);

    int frames = 0;
    int chunkIndex = 0;
    while (!empty_internal(increment_index(rd_idx, chunkIndex), wr_idx)) {
      AudioChunk *chunk = &data_.get()[increment_index(rd_idx, chunkIndex)];

      frames += chunk->frames - chunk->consumed;
      chunkIndex++;
    }

    return frames;
  }

  /**
   * Get the number of available elements for consuming.
   *
   * Only safely called on the producer thread.
   *
   * @return The number of empty slots in the buffer, available for writing.
   */
  int available_write() const
  {
#ifndef NDEBUG
    assert_correct_thread(producer_id);
#endif
    return available_write_internal(read_index_.load(std::memory_order::memory_order_relaxed),
                                    write_index_.load(std::memory_order::memory_order_relaxed)) * (CHUNK_SIZE_BYTES / bytesPerFrame);
  }
  /**
   * Get the total capacity, for this ring buffer.
   *
   * Can be called safely on any thread.
   *
   * @return The maximum capacity of this ring buffer.
   */
  int capacity() const
  {
    return storage_capacity() - 1;
  }
  /**
   * Reset the consumer and producer thread identifier, in case the thread are
   * being changed. This has to be externally synchronized. This is no-op when
   * asserts are disabled.
   */
  void reset_thread_ids()
  {
#ifndef NDEBUG
    consumer_id = producer_id = std::thread::id();
#endif
  }
private:
  /** Return true if the ring buffer is empty.
   *
   * @param read_index the read index to consider
   * @param write_index the write index to consider
   * @return true if the ring buffer is empty, false otherwise.
   **/
  bool empty_internal(int read_index,
                      int write_index) const
  {
    return write_index == read_index;
  }
  /** Return true if the ring buffer is full.
   *
   * This happens if the write index is exactly one element behind the read
   * index.
   *
   * @param read_index the read index to consider
   * @param write_index the write index to consider
   * @return true if the ring buffer is full, false otherwise.
   **/
  bool full_internal(int read_index,
                     int write_index) const
  {
    return (write_index + 1) % storage_capacity() == read_index;
  }
  /**
   * Return the size of the storage. It is one more than the number of elements
   * that can be stored in the buffer.
   *
   * @return the number of elements that can be stored in the buffer.
   */
  int storage_capacity() const
  {
    return capacity_;
  }
  /**
   * Returns the number of elements available for reading.
   *
   * @return the number of available elements for reading.
   */
  int
  available_read_internal(int read_index,
                          int write_index) const
  {
    if (write_index >= read_index) {
      return write_index - read_index;
    } else {
      return write_index + storage_capacity() - read_index;
    }
  }
  /**
   * Returns the number of empty elements, available for writing.
   *
   * @return the number of elements that can be written into the array.
   */
  int
  available_write_internal(int read_index,
                           int write_index) const
  {
    /* We substract one element here to always keep at least one sample
     * free in the buffer, to distinguish between full and empty array. */
    int rv = read_index - write_index - 1;
    if (write_index >= read_index) {
      rv += storage_capacity();
    }
    return rv;
  }
  /**
   * Increments an index, wrapping it around the storage.
   *
   * @param index a reference to the index to increment.
   * @param increment the number by which `index` is incremented.
   * @return the new index.
   */
  int
  increment_index(int index, int increment) const
  {
    assert(increment >= 0);
    return (index + increment) % storage_capacity();
  }
  /**
   * @brief This allows checking that enqueue (resp. dequeue) are always called
   * by the right thread.
   *
   * @param id the id of the thread that has called the calling method first.
   */
#ifndef NDEBUG
  static void assert_correct_thread(std::thread::id& id)
  {
    if (id == std::thread::id()) {
      id = std::this_thread::get_id();
      return;
    }
    assert(id == std::this_thread::get_id());
  }
#endif
  /** Index at which the oldest element is at, in samples. */
  std::atomic<int> read_index_;
  /** Index at which to write new elements. `write_index` is always at
   * least one element ahead of `read_index_`. */
  std::atomic<int> write_index_;
  /** Maximum number of elements that can be stored in the ring buffer. */
  const int capacity_;
  const int bytesPerFrame;
  /** Data storage */
  std::unique_ptr<AudioChunk[]> data_;
#ifndef NDEBUG
  /** The id of the only thread that is allowed to read from the queue. */
  mutable std::thread::id consumer_id;
  /** The id of the only thread that is allowed to write from the queue. */
  mutable std::thread::id producer_id;
#endif
};
