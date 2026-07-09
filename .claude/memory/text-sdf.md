---
name: text-sdf
description: Text SDF rendering pipeline and FontManager (current WIP)
metadata: 
  node_type: memory
  type: project
  originSessionId: cb887217-7f5f-482e-985c-00065623dbcf
---

# Text SDF System (Work In Progress)

## Status

Pipeline built, FontManager built, GTextField created. **Not yet verified working end-to-end.**

## Components

### FontManager (font_manager.cpp/h)
- Singleton: `FontManager::Instance()`
- `initialize(device, asyncMgr, config)` — creates SDF texture array
- `addFont(ttfData, size)` → fontID
- `getGlyph(fontID, charCode)` → GlyphInfo (generates SDF on first request)
- `tickUpload(device)` — uploads pending SDF data to GPU (call each frame)
- Uses `stbtt_GetCodepointSDF` for glyph rasterization
- Config: `{sdfSourceSize=64, extraBorder=8, searchDistance=8, tileSize=64, atlasSize=1024, maxLayers=4}`

### TextSDFRender (text_sdf_render.cpp/h)
- Singleton pipeline for text rendering, mirrors UIImageRender
- Shader: `bin/shaders/fgui_text/` (vert = instanced, frag = SDF smoothstep)
- Supports 4 effect types via `props.z`: 0=basic, 1=outline, 2=shadow, 3=inner glow
- UIMeshType::Font routes to this pipeline

### GTextField (g_text_field.cpp/h)
- ObjectType::Text
- Stores properties + writes to ECS `text_desc_t` component
- `syncTextDesc()` → sets mesh_dirty + batch_dirty
- Mesh generated lazily by `updateImageMesh()` → `createTextMesh()`

### createTextMesh (image_mesh.cpp)
- Reads `text_desc_t`, queries FontManager per-character
- Generates `image_mesh_t` with quad per glyph (same vertex format as Image)
- Handles UTF-8 decoding, line breaks

## Initialization (fgui_test.cpp)

```cpp
// FontManager
auto* fm = FontManager::Instance();
fm->initialize(device, asyncLoadMgr, fontCfg);
// Load font from archive
auto fontStream = arch->openIStream("hwzhsong.ttf");
fm->addFont(ttf.data(), ttf.size());  // → _defaultFontID

// Per-frame
fm->tickUpload(device);
```

## Known Issues

1. **First frame blank**: SDF generation happens in `updateImageMesh`, upload happens next frame in `tickUpload`. New glyphs are blank for 1 frame.

2. **args_need_sync mask zeroing**: `updateItemTransforms` clears `Asm_Transform` bit. `syncArgsToBatch` needs to sync even when mask=0. Currently `syncArgsToBatch` still skips if mask=0 after transform clear. **BUG: batch cache not updated after transform-only changes.**

3. **Texture array empty**: FontManager not initialized with `createTexture`. Need to verify VkImage creation. Texture format: `R8_UNORM`, Texture2DArray.

4. **Text mesh build**: `createTextMesh` called in `updateImageMesh` but `image_mesh.cpp` has the function inside namespace scope — need to verify it's accessible.

## TODO

1. Fix args_need_sync / batch cache update for transform-only changes
2. Verify FontManager texture creation and upload
3. Test GTextField in fgui_test with actual font
4. Add multiline support with proper line height
5. Unicode ranges
