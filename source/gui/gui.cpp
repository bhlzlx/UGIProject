#include "gui.h"
#include "core/display_objects/display_components.h"
#include "core/display_objects/display_object.h"
#include "core/n_texture.h"
#include "core/package.h"
#include "core/ui/stage.h"
#include "entt/entity/fwd.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "mesh/image_mesh.h"
#include "render/render_data.h"
#include "render/ui_render.h"
#include "render/text_sdf_render.h"
#include "texture.h"
#include <vector>

#include <core/display_objects/display_object_utility.h>
#include <core/data_types/tween_manager.h>

/**
 * @brief 
 当单个元件的透明度，颜色，旋转发生变化时，往往是不需要去重新构建batch数据的，只需要更新现有batch数据的args参数就可以了
 */

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
        reg.view<dispcomp::visible_dirty>().each([](auto ett) {
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
            reg.remove<dispcomp::visible_dirty>(ett);
        });
    }

    void travalBatchNode(entt::entity ett, std::function<void(entt::entity)>const& callback);

    // 向上遍历找所属 batch_node 并标记 batch_need_rebuild
    static void markBatchNeedRebuild(entt::entity ett) {
        auto p = getParent(ett);
        while (p && !isBatchNode(p)) {
            p = getParent(p);
        }
        if (isBatchNode(p)) {
            reg.emplace_or_replace<dispcomp::batch_need_rebuild>(p);
        }
    }

    void updateBatchNodeTree() {
        // Step 1: 有 batch_dirty 的非 batch_node → 找父 batch_node → 标记 batch_need_rebuild
        reg.view<dispcomp::batch_dirty, dispcomp::final_visible>(entt::exclude<dispcomp::batch_node>).each([&](entt::entity ett) {
            markBatchNeedRebuild(ett);
            reg.remove<dispcomp::batch_dirty>(ett);
        });

        // Step 2: 有 batch_dirty 的 batch_node → 重建 children / batchNodes 列表
        reg.view<dispcomp::final_visible, dispcomp::batch_node, dispcomp::batch_dirty>().each([&](entt::entity ett, dispcomp::batch_node& bn) {
            bn.children.clear();
            bn.batchNodes.clear();
            travalBatchNode(ett, [&](entt::entity child) {
                bn.children.push_back(child);
                if (isBatchNode(child)) {
                    bn.batchNodes.push_back(child);
                }
                reg.emplace_or_replace<dispcomp::item_batch_info>(child, ett, -1);
                // 结构变化 → 所有 item 标记需要重算 local-to-batch
                if (reg.any_of<dispcomp::item_render_data>(child)) {
                    auto& s = reg.get_or_emplace<dispcomp::args_need_sync>(child);
                    s.mask |= dispcomp::Asm_Transform;
                }
            });
            reg.remove<dispcomp::batch_dirty>(ett);
            reg.emplace_or_replace<dispcomp::batch_need_rebuild>(ett);
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

    struct material_batch_desc_t {
        UIMeshType renderType = UIMeshType::None;
        ugi::Texture* texture = nullptr; 
        //
        bool compatible(UIMeshType type, ugi::Texture* tex) const {
            return renderType == type && texture == tex;
        }
    };

    void rebuildBatches() {
        reg.view<dispcomp::final_visible, dispcomp::batch_need_rebuild, dispcomp::batch_node>().each([](entt::entity ett, dispcomp::batch_node& batchNode) {
            material_batch_desc_t material;
            std::vector<ui_render_batches_t> batches;
            //
            std::vector<void*> renderItems;
            std::vector<item_args_t*> args;

            auto breakBatchFn = [&]() {
                if(renderItems.size()) {
                    switch (material.renderType) {
                    case UIMeshType::None:
                    case UIMeshType::Image: {
                        auto batch = BuildImageRenderBatches(renderItems, args, material.texture);
                        batch.batchNode = ett;
                        batches.push_back(batch);
                        break;
                    }
                    case UIMeshType::Font: {
                        auto batch = BuildTextRenderBatches(renderItems, args, material.texture);
                        batch.batchNode = ett;
                        batches.push_back(batch);
                        break;
                    }
                    case UIMeshType::SubBatch:
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
                DisplayObject obj(child);
                auto& parentBatch = obj.getParentBatch(); // 一定存在
                if(!isBatchNode(child)) {
                    auto graphics = getRenderResource(child);
                    if(!graphics) { // 没有渲染内容，跳过，像普通的component
                        continue;
                    }
                    ugi::Texture* tex = nullptr;
                    auto ntex = graphics->texture.as<NTexture>();
                    if(ntex) {
                        tex = ntex->nativeTexture().as<ugi::Texture>();
                    }
                    if(!tex) {
                        tex = Package::EmptyTexture();
                    }
                    if(!material.compatible(graphics->meshData.type, tex) && renderItems.size()) {
                        breakBatchFn();
                    }
                    material.renderType = graphics->meshData.type;
                    material.texture = tex;
                    if(graphics->meshData.item) {
                        renderItems.push_back((void*)graphics->meshData.item);
                        args.push_back(&graphics->args);
                    }
                    // rebuild 已同步全部 args，后续 syncDirtyArgs 可跳过此 item
                    reg.remove<dispcomp::args_need_sync>(child);
                    parentBatch.instIndex = (int)args.size() - 1; // 更新索引
                    parentBatch.batchIdx  = (int)batches.size();  // 当前所在 sub-batch
                } else {
                    breakBatchFn(); // 遇到batch_root也强行中断
                    material.renderType = UIMeshType::None;
                    material.texture = nullptr;
                    //
                    ui_render_batches_t subBatch;
                    subBatch.type = UIMeshType::SubBatch;
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
                dispcomp::batch_data& batchData = reg.emplace_or_replace<dispcomp::batch_data>(ett);
                batchData.batches = std::move(batches);
            }
            // 重建了，就不是脏的了
            reg.remove<dispcomp::batch_need_rebuild>(ett);
        });
    }

    void commitBatchNode(entt::entity ett, glm::mat4 const& parentWorld) {
        auto batchData = getBatchData(ett);
        if(!batchData) {
            return;
        }
        glm::mat4 localMat(1.0f);
        if (reg.any_of<dispcomp::batch_local_matrix>(ett)) {
            localMat = reg.get<dispcomp::batch_local_matrix>(ett).mat;
        }
        glm::mat4 batchWorld = parentWorld * localMat;
        for(auto& batch: batchData->batches) {
            if(batch.type != UIMeshType::SubBatch) {
                CommitRenderBatch(batch, batchWorld);
            } else {
                commitBatchNode(batch.batchNode, batchWorld);
            }
        }
    }

    void updateLocalMatrix() {
        reg.view<dispcomp::transform_dirty, dispcomp::batch_node>().each([](entt::entity ett, dispcomp::batch_node&) {
            auto& cache = reg.get_or_emplace<dispcomp::batch_local_matrix>(ett);
            cache.mat = buildLocalMatrix(ett);
            reg.remove<dispcomp::transform_dirty>(ett);
        });
    }

    void commitRenderBatches() {
        ClearFrameBatchCache();
        auto stage = Stage::Instance();
        auto root = stage->defaultRoot();
        commitBatchNode(root->getDisplayObject(), glm::mat4(1.0f));
    }

    void GuiTick() {
        updateVisible(); // 更新可见性
        updateBatchNodeTree(); // 维护 batch_node 树结构，传播 dirty 标记
        updateImageMesh(); // 有必要就更新mesh
        updateTextAlignment(); // 根据 text_bounds 重新计算对齐偏移
        updateLocalMatrix(); // batch node 自身矩阵有变化时重算缓存
        updateItemTransforms(); // item 的 Asm_Transform → 重算 local-to-batch 矩阵
        rebuildBatches(); // 重建 batch → 新缓存 + 新索引
        syncDirtyArgs(); // 用新索引同步 args_dirty 到 batch cache（上一行才建好的）
        TweenManager::Instance()->update(); // 驱动所有活跃 Tween
        commitRenderBatches(); // 按渲染顺序提交 batch
    }
    
}