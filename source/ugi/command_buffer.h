﻿#pragma once
#include "resource.h"
#include "ugi_types.h"
#include "command_encoder/compute_cmd_encoder.h"
#include "command_encoder/render_cmd_encoder.h"
#include "command_encoder/resource_cmd_encoder.h"
#include <cstdint>

namespace ugi {

  class CommandBuffer;

  class ResourceCommandEncoder;
  class RenderCommandEncoder;

  class CommandBuffer {
      friend class CommandQueue;
  private:
      VkCommandBuffer   _cmdbuff;
      CmdbufType        _type;
      VkDevice          _device;
      union {
          uint32_t _encodeState;
          ResourceCommandEncoder _resourceEncoder;
          RenderCommandEncoder _renderEncoder;
          ComputeCommandEncoder _computeEncoder;
      };
  private:
      ~CommandBuffer() {
        _type = CmdbufType::Transient;
      }

  public:
      CommandBuffer(VkDevice device, VkCommandBuffer cb, CmdbufType type);
      //
      operator VkCommandBuffer() const;
      VkDevice device() const;
      CmdbufType type() const {
          return _type;
      }

      ResourceCommandEncoder* resourceCommandEncoder();
      RenderCommandEncoder* renderCommandEncoder(IRenderPass* renderPass);
      ComputeCommandEncoder* computeCommandEncoder();
      //
      void reset();
      void beginEncode();
      void endEncode();
  };

}