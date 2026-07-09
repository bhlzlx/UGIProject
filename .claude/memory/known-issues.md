---
name: known-issues
description: Known bugs and important notes
metadata: 
  node_type: memory
  type: project
  originSessionId: cb887217-7f5f-482e-985c-00065623dbcf
---

# Known Issues & Important Notes

## Active Bugs

### 1. args_need_sync mask lost during sync
- `updateItemTransforms` clears `Asm_Transform` → mask becomes 0
- `syncArgsToBatch` or `syncDirtyArgs` skips when mask=0
- Batch cache doesn't get updated after rotation/position change
- **Fix**: sync should always copy when `args_need_sync` component exists, mask only used to gate `accumulateLocalToBatch`

### 2. args_need_sync struct default value
```cpp
struct args_need_sync { uint8_t mask; };  // NO "= 0"!
```
Adding `= 0` makes the struct non-trivial in EnTT, causing mask to be zeroed during sparse set realloc.

### 3. Text mesh blank first frame
FontManager generates SDF CPU-side but uploads to GPU next frame.

## Design Notes

- EnTT 3.13.0: `each()` callback params = entity + non-empty components. Tags (empty structs like `batch_dirty`, `mesh_dirty`) are NOT passed.
- `Mesh::CreateMesh` offset fix: `attriOffsets_[i] = alloc.offset + layout.buffers[i].offset` — MUST include alloc.offset.
- `GPURetireManager` delays GPU resource destruction by 3 frames (MaxFlightCount).
- `Renderable::release()` → GPURetireManager → delete after 3 frames.

## File Reorganization Wanted
- `item_resource_t` renamed to `dispcomp::item_render_data` (done)
- `dispcomp::graphics` wrapper removed (done)
- `args_dirty` → `args_need_sync` with bitmask (done, but WIP)
