---
name: transform-system
description: Transform matrix computation and batch world accumulation
metadata: 
  node_type: memory
  type: project
  originSessionId: cb887217-7f5f-482e-985c-00065623dbcf
---

# Transform System

## Local Matrix

`buildLocalMatrix(ett)` in display_object_utility.cpp:
```
M = T(pos) * R(rot) * Skew(skew) * S(scale) * P(-pivot*size)
```
Order: pivot → scale → skew → rotate → translate (applied to vertex).

## Batch Node Matrix

`batch_local_matrix` component caches `buildLocalMatrix(ett)` for batch nodes. Updated in GuiTick when `transform_dirty` + `batch_node`.

## Item Local-to-Batch

`accumulateLocalToBatch(item, batch)` walks parent chain from item up to batch node, multiplies all `buildLocalMatrix()` along the path. Called by `updateItemTransforms()` when `Asm_Transform` flag is set.

## Commit-Traversal

`commitBatchNode(ett, parentWorld)` recursively computes batch world:
```
batchWorld = parentWorld * batch_local_matrix(mat)
```
Passed to `CommitRenderBatch(batch, batchWorld)` → shader's `batchWorld` uniform.

## Property Change Flow

```
setRotation/Position/Scale
  → dispobj_.setRotation/etc → transform_dirty + args_need_sync(Asm_Transform)

GuiTick:
  updateLocalMatrix → batch nodes with transform_dirty → recompute cache
  updateItemTransforms → items with Asm_Transform → accumulateLocalToBatch → gfx.args.transfrom
  syncDirtyArgs → copy gfx.args to batch cache → remove args_need_sync
```

## UIContentScaler

- `UIContentScaler::Instance()` — global singleton
- `scaleFactor = min(screenW/dx, screenH/dy)` (MatchWidthOrHeight mode)
- `applyContentScaleFactor()` → root.setSize(logicalW, logicalH) + root.setScale(sf, sf)
- Root's scale component propagates through `buildLocalMatrix` → `batch_local_matrix`

## Important: args_need_sync struct

```cpp
struct args_need_sync {
    uint8_t mask;  // NO default value! EnTT aggregate init preserves value during move
};
```
Default `= 0` breaks EnTT move semantics — mask gets zeroed during sparse set realloc.
