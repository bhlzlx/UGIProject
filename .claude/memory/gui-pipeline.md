---
name: gui-pipeline
description: GUI rendering pipeline and GuiTick flow
metadata: 
  node_type: memory
  type: project
  originSessionId: cb887217-7f5f-482e-985c-00065623dbcf
---

# GUI Rendering Pipeline

## GuiTick Flow (gui.cpp)

```cpp
void GuiTick() {
    updateVisible();           // visibility propagation
    updateBatchNodeTree();     // batch tree structure + dirty propagation
    updateImageMesh();         // Image/Text mesh generation (entity → mesh)
    updateLocalMatrix();       // batch node matrix cache
    updateItemTransforms();    // local-to-batch transform recalculation
    rebuildBatches();          // GPU batch construction (mesh + args → buffer)
    syncDirtyArgs();           // incremental args sync to batch cache
    commitRenderBatches();     // submit to GPU
}
```

## Key Design Decisions

1. **ECS deferred mesh generation**: Image and Text write descriptors to ECS (`image_desc_t`/`text_desc_t`), set `mesh_dirty`. `updateImageMesh()` processes dirty entities and creates `image_mesh_t` in `item_render_data::meshData`.

2. **Batch stores entity handles (not pointers)** for safety:
   - Batch stores `std::vector<entt::entity>` instead of raw pointers
   - `drawBatch()` reads `reg.get<item_render_data>(entity).args` each frame
   - EnTT sparse set reallocations don't invalidate entity handles

3. **Instance-draw batching**: All sprites in same batch use instanced rendering with per-instance UBO data (`item_args_t = transform + color + props`). Max 512 instances per GPU batch.

4. **Shader**: `vp * batchWorld * localTransform * vertex` — batchWorld computed during commit traversal, localTransform per instance.

## Rendering Layers

```
Application tick
  → GuiTick() (CPU: entity updates, mesh gen, batch building)
  → commitRenderBatches() (GPU: batch submission)
  → DrawRenderBatches() (GPU: per-batch draw calls)
```

## Batch Lifecycle

```
batch_dirty set on entity
  → updateBatchNodeTree → batch_need_rebuild on parent batch node
  → rebuildBatches:
       destroy old GPU batches (Renderable::release → GPURetireManager)
       collect items → build merged vertex buffer → createRenderable (async GPU upload)
       store new batch_data on batch node
       clear args_need_sync from processed items
  → commitRenderBatches:
       walk batch tree → compute batchWorld → CommitRenderBatch(batch, batchWorld)
  → DrawRenderBatches:
       allocate UBO → memcpy cachedArgs → bind descriptors → vkCmdDrawIndexed
```
