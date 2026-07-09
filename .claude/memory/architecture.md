---
name: architecture
description: Project architecture overview
metadata: 
  node_type: memory
  type: project
  originSessionId: cb887217-7f5f-482e-985c-00065623dbcf
---

# Project Architecture

## Overview

C++ FairyGUI port (from C# FairyGUI-unity) on custom Vulkan renderer (UGI) + EnTT ECS.

## Directory Layout

```
source/gui/
├── core/                   # Core GUI logic
│   ├── ui/                 # Object hierarchy (GObject→Component→Image/Text/Root)
│   ├── display_objects/    # ECS components + utilities
│   ├── data_types/         # Relation, Transition, Tweener, Value
│   ├── events/             # EventDispatcher, InputEvent
│   ├── declare.h           # Core enums (ObjectType, RelationType, etc.)
│   ├── package.cpp/h       # .fui package loading
│   ├── package_item.cpp/h  # Individual package asset
│   ├── font_manager.cpp/h  # SDF font glyph generation (NEW - WIP)
│   └── controller.cpp/h    # Controller state machine
├── render/                 # GPU rendering
│   ├── ui_image_render.cpp/h  # Image pipeline (instanced quad rendering)
│   ├── text_sdf_render.cpp/h  # Text SDF pipeline (NEW - WIP)
│   ├── ui_render.cpp/h        # Batch commit/dispatch
│   └── render_data.h          # Vertex/instance/batch data structures
├── mesh/                   # Mesh generation
│   └── image_mesh.cpp/h    # Quad mesh generation (Image: plain/9slice/tile + Text)
└── gui.cpp                 # Main tick: GuiTick(), transform, batch management

source/ugi/                 # Vulkan renderer (UGI)
│   ├── render_components/  # Mesh, Renderable, Material
│   ├── asyncload/          # GPUAsyncLoadManager
│   ├── mesh_buffer_allocator.cpp/h  # TLSF-based GPU buffer allocator
│   └── render_context.cpp/h  # Main render loop
```

## ECS Architecture (EnTT)

UI Objects (GObject) own DisplayObject entities in ECS. Key components in `dispcomp` namespace:

**Entity tree:**
- `children` — vector of child entities
- `parent` — parent entity
- `is_root` — root marker

**Batch system:**
- `batch_node` — batch root (stops traversal)
- `batch_data` — vector of GPU batches
- `batch_dirty` — need tree structure update
- `batch_need_rebuild` — need GPU batch rebuild
- `item_batch_info` — {batchEntity, batchIdx, instIndex}

**Rendering:**
- `item_render_data` — {meshData, args, texture}
- `image_desc_t` — image mesh parameters
- `text_desc_t` — text mesh parameters (NEW)
- `mesh_dirty` — need mesh rebuild

**Transform:**
- `basic_transform` — position, size, pivot
- `scale`, `rotation`, `skew` — optional transform components
- `transform_dirty` — transform needs update
- `batch_local_matrix` — cached local matrix for batch nodes
- `args_need_sync` — bitmask: {Asm_Transform, Asm_Color, Asm_Props}

**Visibility:**
- `visible`, `final_visible`, `visible_dirty`
