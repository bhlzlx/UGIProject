#include "gui.h"
#include "core/display_objects/display_components.h"
#include "core/display_objects/display_object.h"
#include "core/ui/stage.h"
#include "entt/entity/fwd.hpp"
#include "mesh/image_mesh.h"
#include "render/render_data.h"
#include "render/ui_render.h"
#include "texture.h"
#include <vector>

#include <core/display_objects/display_object_utility.h>

namespace gui {

    void updateVisibleRecursive(entt::entity ett, bool parentVisible) {
        bool visible = isVisible(ett);
        bool finalVisible = parentVisible && visible;
        setFinalVisible(ett, finalVisible);
        if(isBatchNode(ett)) { // 到batch node就不要再往下更新了
            return;
        }
        auto children = getChildren(ett);
        if(children) {
            for(auto child: children->val) {
                updateVisibleRecursive(child, finalVisible);
            }
        }
    }

    void updateVisible() {
        auto stage = Stage::Instance();
        auto root = stage->defaultRoot();
        //
        reg.view<dispcomp::visible_changed>().each([](auto ett) {
            bool isRoot = false;
            isRoot = reg.any_of<dispcomp::is_root>(ett);
            auto parent = getParent(ett);
            if(!isBatchNode(ett) && parent && isRoot) {  // 没有父控件，而且是脏的，现在不是更新它的时候
                return;
            } else {
                if(parent) {
                    updateVisibleRecursive(ett,isFinalVisible(parent));
                // } else {
                //     updateVisibleRecursive(ett, true);
                }
            }
            reg.remove<dispcomp::visible_changed>(ett);
        });
    }

    void travalBatchNode(entt::entity ett, std::function<void(entt::entity)>const& callback) {
        auto children = getChildren(ett);
        if(!children) {
            callback(ett);
            return;
        } else {
            for(auto child: children->val) { // 按渲染顺序遍历
                if(!isBatchNode(child)) { // 遇到batch node节点终止
                    travalBatchNode(child, callback);
                }
            }
        }
    }

    void updateBatchNodes() {
        reg.view<dispcomp::batch_dirty, dispcomp::batch_node>().each([=](entt::entity ett, dispcomp::batch_node& batchNode) {
            batchNode.children.clear();
            batchNode.batchNodes.clear();
            travalBatchNode(ett, [&batchNode](entt::entity ett) {
                batchNode.children.push_back(ett);
                if(isBatchNode(ett)) {
                    batchNode.batchNodes.push_back(ett);
                }
            });
        });
    }

    struct material_batch_desc_t {
        RenderItemType renderType = RenderItemType::None;
        ugi::Texture* texture = nullptr; 
        //
        bool compatible(RenderItemType type, ugi::Texture* tex) const {
            return renderType == type && texture == tex;
        }
    };

    void updateDirtyBatches() {
        reg.view<dispcomp::final_visible, dispcomp::batch_dirty, dispcomp::batch_node>().each([](entt::entity ett, dispcomp::batch_node& batchNode) {
            material_batch_desc_t material;
            std::vector<ui_render_batches_t> batches;
            //
            std::vector<void*> renderItems;
            std::vector<ui_inst_data_t*> args;

            auto breakBatchFn = [&]() {
                if(renderItems.size()) {
                    switch (material.renderType) {
                    case RenderItemType::None:
                    case RenderItemType::Image: {
                        auto batch = BuildImageRenderBatches(renderItems, args, material.texture);
                        batches.push_back(batch);
                        break;
                    }
                    case RenderItemType::Font:
                    break;
                    case RenderItemType::SubBatch:
                    break;
                    }
                }
                renderItems.clear();
                args.clear();
            };

            for(auto child: batchNode.children) {
                if(!isFinalVisible(child)) { // 只有可见的才挂到渲染树上，不要遍历的时候再去判断，影响性能
                    continue;
                }
                if(!isBatchNode(child)) {
                    auto graphics = getGraphics(child);
                    ugi::Texture* tex = graphics->texture.nativeTexture().as<ugi::Texture>();
                    if(!material.compatible(graphics->renderItem.type, tex) && renderItems.size()) {
                        breakBatchFn();
                        material.renderType = graphics->renderItem.type;
                        material.texture = tex;
                    }
                    renderItems.push_back((void*)graphics->renderItem.item);
                    args.push_back(&graphics->args);
                } else {
                    breakBatchFn(); // 遇到batch_root也强行中断
                    material.renderType = RenderItemType::None;
                    material.texture = nullptr;
                    //
                    ui_render_batches_t subBatch;
                    subBatch.type = RenderItemType::SubBatch;
                    subBatch.batchNode = child;
                    batches.push_back(subBatch); // 子batch让它自己去管自己
                }
            }
            breakBatchFn();
            //
            dispcomp::batch_data* oldBatches = getBatchData(ett);
            if(oldBatches) {
                for(auto const& batch: oldBatches->batches) {
                    DestroyRenderBatches(batch);
                }
            }
            if(batches.size()) {
                dispcomp::batch_data& batch = reg.emplace_or_replace<dispcomp::batch_data>(ett);
                batch.batches = std::move(batches);
            }
            // 重建了，就不是脏的了
            reg.remove<dispcomp::batch_dirty>(ett);
        });
    }

    void commitBatchNode(entt::entity ett) {
        auto batchData = getBatchData(ett);
        if(!batchData) {
            return;
        }
        for(auto batch: batchData->batches) {
            if(batch.type != RenderItemType::SubBatch) {
                CommitRenderBatch(batch);
            } else {
                commitBatchNode(batch.batchNode);
            }
        }
    }

    void commitRenderBatches() {
        ClearFrameBatchCache();
        auto stage = Stage::Instance();
        auto root = stage->defaultRoot();
        commitBatchNode(root->getDisplayObject());
    }

    void GuiTick() {
        updateVisible(); // 更新可见性
        updateImageMesh(); // 有必要就更新mesh
        updateBatchNodes();
        updateDirtyBatches(); // 更新batch节点的 batch 数据
        commitRenderBatches(); // 按渲染顺序提交 batch 
    }
    
}