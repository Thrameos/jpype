// --- file: org/jpype/ref/GlobalPool.java ---
/* ****************************************************************************
  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  See NOTICE file for details.
**************************************************************************** */
package org.jpype.ref;

import java.util.ArrayDeque;
import java.util.ArrayList;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.atomic.AtomicInteger;

// FIXME this has multiple responsibilities registry and individual pool
// FIXME this has a large shift from j2ni/J2GlobalPool design.  Evaluate if copying closer would help.

/**
 * Per-interpreter pool of Java object references, addressed by opaque long
 * handles instead of held directly by native code.
 *
 * <p>
 * Native (C++) type wrappers (JPClass, JPMethod, JPField, ...) that used to
 * hold a raw JNI global reference hold one of these handles instead.
 * Ordinary Java reachability from this pool's own array keeps the object
 * alive, so no {@code NewGlobalRef}/{@code DeleteGlobalRef} is needed for
 * them at all - the object is just a normal Java reference for as long as
 * this pool holds it.</p>
 *
 * <p>
 * Each pool stamps its own prefix - assigned once, at construction, from
 * the static counter below, and recorded in the static registry - into
 * every handle it mints, alongside a generation counter per slot. A handle
 * therefore self-identifies both its owning pool and whether the slot it
 * names has since been reused. {@link #tryRelease} is a static,
 * pool-agnostic entry point that routes purely off those bits - the one
 * case that needs it is native code (a C++ destructor) releasing a handle
 * with no other live context in hand.</p>
 */
public final class GlobalPool
{

  private static final int PREFIX_SHIFT = 48;
  private static final int GENERATION_SHIFT = 32;
  private static final long INDEX_MASK = 0xFFFFFFFFL;
  private static final long GENERATION_MASK = 0xFFFFL;
  private static final long PREFIX_MASK = 0xFFFFL;

  private static final AtomicInteger NEXT_PREFIX = new AtomicInteger(1);
  private static final Map<Integer, GlobalPool> REGISTRY = new ConcurrentHashMap<>();

  private final int prefix;
  private final ArrayList<Object> slots = new ArrayList<>();
  private final ArrayList<Integer> generations = new ArrayList<>();
  private final ArrayDeque<Integer> freeList = new ArrayDeque<>();

  /**
   * Constructs a new pool and registers it under a freshly-assigned
   * prefix, unique among currently-registered pools.
   */
  public GlobalPool()
  {
    int id = NEXT_PREFIX.getAndIncrement();
    if ((id & (int) PREFIX_MASK) == 0)
    {
      // 0 is reserved - never a valid prefix - so skip it on wraparound.
      id = NEXT_PREFIX.getAndIncrement();
    }
    this.prefix = id & (int) PREFIX_MASK;
    REGISTRY.put(this.prefix, this);
  }

  /**
   * Removes this pool from the static registry and drops every slot.
   *
   * <p>
   * Called once, from the owning interpreter's own shutdown. After this,
   * {@link #tryRelease} for any handle this pool ever minted safely
   * no-ops instead of touching a dead pool, and {@link #get} likewise
   * returns {@code null}.</p>
   */
  public synchronized void close()
  {
    REGISTRY.remove(prefix);
    slots.clear();
    generations.clear();
    freeList.clear();
  }

  /**
   * Stores obj and returns a self-checking handle for it.
   *
   * <p>
   * {@code null} is special-cased to the handle {@code 0} without
   * allocating a slot - this lets native code test "is this Java value
   * null" as a cheap {@code == 0} on the handle itself, with no pool
   * lookup/JNI call required (see JPValue::isJavaNull in jp_value.h).</p>
   *
   * @param obj The object to hold a reference to.
   * @return The handle, suitable for {@link #get}/{@link #tryRelease}.
   */
  public synchronized long add(Object obj)
  {
    if (obj == null)
      return 0L;
    int index;
    int generation;
    if (!freeList.isEmpty())
    {
      index = freeList.removeLast();
      generation = generations.get(index) + 1;
      generations.set(index, generation);
      slots.set(index, obj);
    } else
    {
      index = slots.size();
      generation = 0;
      slots.add(obj);
      generations.add(0);
    }
    return encode(prefix, generation, index);
  }

  /**
   * Resolves handle back to the object it names.
   *
   * @param handle A handle previously returned by {@link #add}.
   * @return The object, or {@code null} if handle doesn't belong to this
   * pool, or names a slot that has since been released/reused.
   */
  public synchronized Object get(long handle)
  {
    if (decodePrefix(handle) != prefix)
    {
      return null;
    }
    int index = decodeIndex(handle);
    if (index < 0 || index >= slots.size())
    {
      return null;
    }
    if (generations.get(index) != decodeGeneration(handle))
    {
      return null;
    }
    return slots.get(index);
  }

  private synchronized void remove(long handle)
  {
    int index = decodeIndex(handle);
    if (index < 0 || index >= slots.size())
    {
      return;
    }
    if (generations.get(index) != decodeGeneration(handle))
    {
      return;
    }
    slots.set(index, null);
    freeList.addLast(index);
  }

  /**
   * Releases a handle, regardless of which pool minted it - the one entry
   * point callable from anywhere (e.g. a C++ destructor with no other
   * live context in hand). Decodes handle's prefix, routes to that pool
   * if it's still registered, and safely no-ops if that interpreter has
   * already torn down.
   *
   * @param handle A handle previously returned by some pool's
   * {@link #add}.
   */
  public static void tryRelease(long handle)
  {
    GlobalPool pool = REGISTRY.get(decodePrefix(handle));
    if (pool != null)
    {
      pool.remove(handle);
    }
  }

  private static long encode(int prefix, int generation, int index)
  {
    return ((prefix & PREFIX_MASK) << PREFIX_SHIFT)
            | ((generation & GENERATION_MASK) << GENERATION_SHIFT)
            | (index & INDEX_MASK);
  }

  private static int decodePrefix(long handle)
  {
    return (int) ((handle >>> PREFIX_SHIFT) & PREFIX_MASK);
  }

  private static int decodeGeneration(long handle)
  {
    return (int) ((handle >>> GENERATION_SHIFT) & GENERATION_MASK);
  }

  private static int decodeIndex(long handle)
  {
    return (int) (handle & INDEX_MASK);
  }
}
