#include "g_loader.h"
#include "core/declare.h"
#include "core/display_objects/display_components.h"
#include "core/display_objects/display_object.h"
#include "core/package.h"
#include "core/package_item.h"
#include "core/ui/component.h"
#include "render/render_data.h"
#include "utils/byte_buffer.h"
#include <algorithm>
#include <cmath>

namespace gui {

    // ============================================================
    // Construction / Destruction
    // ============================================================

    GLoader::GLoader()
        : Object()
        , url_()
        , align_(AlignType::Left)
        , verticalAlign_(VertAlignType::Top)
        , autoSize_(false)
        , fill_(LoaderFillType::None)
        , shrinkOnly_(false)
        , useResize_(false)
        , updatingLayout_(false)
        , color_(0xFFFFFFFF)
        , contentDispObj_()
        , content2_(nullptr)
        , errorSign_(nullptr)
        , showErrorSign_(true)
        , playing_(true)
        , frame_(0)
        , timeScale_(1.0f)
    {
        type_ = ObjectType::Loader;
    }

    GLoader::~GLoader() {
        clearContent();
        if (errorSign_) {
            delete errorSign_;
            errorSign_ = nullptr;
        }
    }

    // ============================================================
    // Display Object
    // ============================================================

    void GLoader::createDisplayObject() {
        Object::createDisplayObject();
        // dispobj_ is the container; content will be added as a child on load.
    }

    // ============================================================
    // URL
    // ============================================================

    bool GLoader::parseURL(std::string const& url,
                           std::string& outPkgID,
                           std::string& outItemName)
    {
        // Format: "ui://PackageID/ItemName"
        const std::string prefix = "ui://";
        if (url.size() < prefix.size() || url.compare(0, prefix.size(), prefix) != 0) {
            return false;
        }
        auto slashPos = url.find('/', prefix.size());
        if (slashPos == std::string::npos || slashPos == prefix.size()) {
            return false;
        }
        outPkgID    = url.substr(prefix.size(), slashPos - prefix.size());
        outItemName = url.substr(slashPos + 1);
        return !outPkgID.empty() && !outItemName.empty();
    }

    void GLoader::setURL(std::string const& val) {
        if (url_ == val) return;

        clearContent();
        url_ = val;
        loadContent();
        updateGear(7);  // Icon gear
    }

    // ============================================================
    // Content Loading
    // ============================================================

    void GLoader::loadContent() {
        clearContent();

        if (url_.empty()) return;

        if (url_.find("ui://") == 0) {
            loadFromPackage(url_);
        } else {
            loadExternal();
        }
    }

    void GLoader::loadFromPackage(std::string const& itemURL) {
        std::string pkgID, itemName;
        if (!parseURL(itemURL, pkgID, itemName)) {
            setErrorState();
            return;
        }

        // Locate the package.
        Package* pkg = PackageForID(pkgID);
        if (!pkg) {
            pkg = PackageForName(pkgID);
        }
        if (!pkg) {
            setErrorState();
            return;
        }

        // Find the item.
        PackageItem* item = pkg->itemByID(itemName);
        if (!item) {
            setErrorState();
            return;
        }

        contentItem_ = item->getBranch();
        sourceSize_.width  = (float)contentItem_->width_;
        sourceSize_.height = (float)contentItem_->height_;
        contentItem_ = contentItem_->getHighSolution();
        contentItem_->load();

        switch (contentItem_->type_) {

        case PackageItemType::Image: {
            // ---- Image content ----
            // Create a child display object for the image.
            contentDispObj_ = DisplayObject::createDisplayObject();
            auto& imgDesc  = reg.get_or_emplace<dispcomp::image_desc_t>(contentDispObj_);
            auto& gfx      = reg.get_or_emplace<dispcomp::item_render_data>(contentDispObj_);
            gfx.args.colorPacked = color_;  // always set, even if texture is null

            NTexture* tex = contentItem_->texture_;
            if (tex) {
                auto const& uvRc = tex->uvRc();
                imgDesc.textureBlock.uv[0] = uvRc.base;
                imgDesc.textureBlock.uv[1] = {uvRc.right(), uvRc.bottom()};
                imgDesc.textureBlock.size.x = (float)contentItem_->width_;
                imgDesc.textureBlock.size.y = (float)contentItem_->height_;
                imgDesc.grid9       = contentItem_->scale9Grid_;
                imgDesc.scaleByTile = contentItem_->scaledByTile_;
                gfx.texture         = tex->handle();
            }

            // Apply pending fill method data (set via setupBeforeAdd or setters).
            imgDesc.ext.fill          = pendingFillMethod_;
            imgDesc.ext.fillOrig      = pendingFillOrigin_;
            imgDesc.ext.fillClockwise = pendingFillClockwise_;
            imgDesc.ext.fillAmount    = pendingFillAmount_;

            reg.emplace_or_replace<dispcomp::mesh_dirty>(contentDispObj_);
            reg.emplace_or_replace<dispcomp::batch_dirty>(contentDispObj_);
            reg.emplace_or_replace<dispcomp::visible>(contentDispObj_);
            reg.emplace_or_replace<dispcomp::visible_dirty>(contentDispObj_);

            dispobj_.addChild(contentDispObj_);
            updateLayout();
            break;
        }

        case PackageItemType::MovieClip: {
            // ---- MovieClip content (stub) ----
            // For now, just load the first frame if the item has a texture.
            contentDispObj_ = DisplayObject::createDisplayObject();
            auto& imgDesc = reg.get_or_emplace<dispcomp::image_desc_t>(contentDispObj_);
            auto& gfx     = reg.get_or_emplace<dispcomp::item_render_data>(contentDispObj_);
            gfx.args.colorPacked = color_;  // always set, even if texture is null

            NTexture* tex = contentItem_->texture_;
            if (tex) {
                auto const& uvRc = tex->uvRc();
                imgDesc.textureBlock.uv[0] = uvRc.base;
                imgDesc.textureBlock.uv[1] = {uvRc.right(), uvRc.bottom()};
                imgDesc.textureBlock.size.x = (float)contentItem_->width_;
                imgDesc.textureBlock.size.y = (float)contentItem_->height_;
                gfx.texture = tex->handle();
            }

            // Apply pending fill method data.
            imgDesc.ext.fill          = pendingFillMethod_;
            imgDesc.ext.fillOrig      = pendingFillOrigin_;
            imgDesc.ext.fillClockwise = pendingFillClockwise_;
            imgDesc.ext.fillAmount    = pendingFillAmount_;

            reg.emplace_or_replace<dispcomp::mesh_dirty>(contentDispObj_);
            reg.emplace_or_replace<dispcomp::batch_dirty>(contentDispObj_);
            reg.emplace_or_replace<dispcomp::visible>(contentDispObj_);
            reg.emplace_or_replace<dispcomp::visible_dirty>(contentDispObj_);

            dispobj_.addChild(contentDispObj_);
            updateLayout();
            break;
        }

        case PackageItemType::Component: {
            // ---- Component content ----
            Object* obj = pkg->createObject(itemName);
            if (!obj || obj->objectType() != ObjectType::Component) {
                if (obj) delete obj;
                if (autoSize_)
                    setSize({(float)contentItem_->width_, (float)contentItem_->height_});
                setErrorState();
                break;
            }
            content2_ = obj;
            // Add the component's display object as a child of GLoader's display object.
            dispobj_.addChild(content2_->getDisplayObject());
            updateLayout();
            break;
        }

        default:
            if (autoSize_)
                setSize({(float)contentItem_->width_, (float)contentItem_->height_});
            setErrorState();
            break;
        }
    }

    void GLoader::loadExternal() {
        // External texture loading — not yet implemented.
        // Subclasses / Puerts hooks will override this.
        setErrorState();
    }

    // ============================================================
    // Content Cleanup
    // ============================================================

    void GLoader::clearContent() {
        clearErrorState();

        if (contentDispObj_) {
            if (contentDispObj_.parent()) {
                contentDispObj_.removeFromParent();
            }
            // Destroy the entity.
            auto e = contentDispObj_.entity();
            if (e != entt::null) {
                reg.destroy(e);
            }
            contentDispObj_ = DisplayObject();
        }

        if (content2_) {
            content2_->removeFromParent();
            delete content2_;
            content2_ = nullptr;
        }

        contentItem_ = nullptr;
    }

    // ============================================================
    // Error State
    // ============================================================

    void GLoader::setErrorState() {
        // TODO: UIConfig.loaderErrorSign — not yet ported.
        // For now this is a no-op.
    }

    void GLoader::clearErrorState() {
        if (errorSign_) {
            auto errDisp = errorSign_->getDisplayObject();
            if (errDisp && errDisp.parent()) {
                dispobj_.removeChild(errDisp);
            }
            delete errorSign_;
            errorSign_ = nullptr;
        }
    }

    // ============================================================
    // Layout
    // ============================================================

    void GLoader::updateLayout() {
        // Nothing to lay out.
        bool hasImageContent  = (contentDispObj_ && contentDispObj_.entity() != entt::null);
        bool hasCompContent   = (content2_ != nullptr);

        if (!hasImageContent && !hasCompContent) {
            if (autoSize_) {
                updatingLayout_ = true;
                setSize({50, 30});
                updatingLayout_ = false;
            }
            return;
        }

        float contentW = sourceSize_.width;
        float contentH = sourceSize_.height;

        if (autoSize_) {
            updatingLayout_ = true;
            if (contentW == 0) contentW = 50;
            if (contentH == 0) contentH = 30;
            setSize({contentW, contentH});
            updatingLayout_ = false;

            if (width() == contentW && height() == contentH) {
                if (hasCompContent) {
                    content2_->setPosition({0, 0, 0});
                    auto* comp = dynamic_cast<Component*>(content2_);
                    if (comp) {
                        comp->setScale(1, 1);
                        if (useResize_)
                            comp->setSize({contentW, contentH});
                    }
                } else {
                    contentDispObj_.setPosition({0, 0});
                    contentDispObj_.setSize({contentW, contentH});
                }
                invalidateBatchingState();
                return;
            }
        }

        // ---- Fill / scale ----
        float sx = 1.0f, sy = 1.0f;
        if (fill_ != LoaderFillType::None) {
            float srcW = sourceSize_.width;
            float srcH = sourceSize_.height;
            if (srcW > 0 && srcH > 0) {
                sx = width()  / srcW;
                sy = height() / srcH;
            }

            if (sx != 1.0f || sy != 1.0f) {
                if (fill_ == LoaderFillType::ScaleMatchHeight)
                    sx = sy;
                else if (fill_ == LoaderFillType::ScaleMatchWidth)
                    sy = sx;
                else if (fill_ == LoaderFillType::Scale) {
                    if (sx > sy) sx = sy;
                    else         sy = sx;
                } else if (fill_ == LoaderFillType::ScaleNoBorder) {
                    if (sx > sy) sy = sx;
                    else         sx = sy;
                }

                if (shrinkOnly_) {
                    if (sx > 1.0f) sx = 1.0f;
                    if (sy > 1.0f) sy = 1.0f;
                }

                contentW = sourceSize_.width  * sx;
                contentH = sourceSize_.height * sy;
            }
        }

        // ---- Position ----
        if (hasCompContent) {
            auto* comp = dynamic_cast<Component*>(content2_);
            if (useResize_ && comp) {
                comp->setScale(1, 1);
                comp->setSize({contentW, contentH});
            } else if (comp) {
                comp->setScale(sx, sy);
            }
        } else {
            contentDispObj_.setSize({contentW, contentH});
        }

        float nx, ny;
        switch (align_) {
        case AlignType::Center: nx = (width() - contentW) * 0.5f; break;
        case AlignType::Right:  nx = width() - contentW;           break;
        default:                nx = 0;                            break;
        }
        switch (verticalAlign_) {
        case VertAlignType::Middle: ny = (height() - contentH) * 0.5f; break;
        case VertAlignType::Bottom: ny = height() - contentH;           break;
        default:                    ny = 0;                             break;
        }

        if (hasCompContent) {
            content2_->setPosition({nx, ny, 0});
        } else {
            contentDispObj_.setPosition({nx, ny});
        }

        invalidateBatchingState();
    }

    void GLoader::onSizeChanged() {
        Object::onSizeChanged();
        if (!updatingLayout_)
            updateLayout();
    }

    // ============================================================
    // Layout Setters
    // ============================================================

    void GLoader::setAlign(AlignType val) {
        if (align_ == val) return;
        align_ = val;
        updateLayout();
    }

    void GLoader::setVerticalAlign(VertAlignType val) {
        if (verticalAlign_ == val) return;
        verticalAlign_ = val;
        updateLayout();
    }

    void GLoader::setFill(LoaderFillType val) {
        if (fill_ == val) return;
        fill_ = val;
        updateLayout();
    }

    void GLoader::setShrinkOnly(bool val) {
        if (shrinkOnly_ == val) return;
        shrinkOnly_ = val;
        updateLayout();
    }

    void GLoader::setAutoSize(bool val) {
        if (autoSize_ == val) return;
        autoSize_ = val;
        updateLayout();
    }

    void GLoader::setUseResize(bool val) {
        if (useResize_ == val) return;
        useResize_ = val;
        updateLayout();
    }

    // ============================================================
    // Color (IColorGear)
    // ============================================================

    void GLoader::setColor(Color4B val) {
        if (color_.val == val.val) return;
        color_ = val;

        // Apply to image/movieclip content.
        if (contentDispObj_ && reg.any_of<dispcomp::item_render_data>(contentDispObj_)) {
            auto& gfx = reg.get<dispcomp::item_render_data>(contentDispObj_);
            gfx.args.colorPacked = color_;
            auto& s = reg.get_or_emplace<dispcomp::args_need_sync>(contentDispObj_);
            s.mask |= dispcomp::Asm_Color;
        }

        updateGear(4);  // Color gear
    }

    // ============================================================
    // MovieClip Controls (stubs)
    // ============================================================

    void GLoader::setPlaying(bool val) {
        playing_ = val;
        updateGear(5);  // Animation gear
    }

    void GLoader::setFrame(int val) {
        frame_ = val;
        updateGear(5);
    }

    void GLoader::setTimeScale(float val) {
        timeScale_ = val;
    }

    // ============================================================
    // Content Accessors
    // ============================================================

    NTexture* GLoader::texture() const {
        // For now, return the texture from the PackageItem if available.
        if (contentItem_)
            return contentItem_->texture_;
        return nullptr;
    }

    void GLoader::setTexture(NTexture* tex) {
        url_.clear();
        clearContent();

        if (tex) {
            contentDispObj_ = DisplayObject::createDisplayObject();
            auto& imgDesc = reg.get_or_emplace<dispcomp::image_desc_t>(contentDispObj_);
            auto& gfx     = reg.get_or_emplace<dispcomp::item_render_data>(contentDispObj_);

            auto const& uvRc = tex->uvRc();
            auto sz = tex->size();
            imgDesc.textureBlock.uv[0] = uvRc.base;
            imgDesc.textureBlock.uv[1] = {uvRc.right(), uvRc.bottom()};
            imgDesc.textureBlock.size  = sz;
            imgDesc.grid9       = nullptr;
            imgDesc.scaleByTile = false;
            gfx.texture         = tex->handle();
            gfx.args.colorPacked = color_;

            sourceSize_ = sz;

            reg.emplace_or_replace<dispcomp::mesh_dirty>(contentDispObj_);
            reg.emplace_or_replace<dispcomp::batch_dirty>(contentDispObj_);
            reg.emplace_or_replace<dispcomp::visible>(contentDispObj_);
            reg.emplace_or_replace<dispcomp::visible_dirty>(contentDispObj_);

            dispobj_.addChild(contentDispObj_);
        } else {
            sourceSize_ = {0, 0};
        }

        updateLayout();
    }

    // ============================================================
    // Fill Method Delegation (Image content only)
    // ============================================================

    FillMethod GLoader::fillMethod() const {
        if (contentDispObj_ && reg.any_of<dispcomp::image_desc_t>(contentDispObj_))
            return reg.get<dispcomp::image_desc_t>(contentDispObj_).ext.fill;
        return FillMethod::None;
    }

    void GLoader::setFillMethod(FillMethod val) {
        pendingFillMethod_ = val;
        if (contentDispObj_ && reg.any_of<dispcomp::image_desc_t>(contentDispObj_)) {
            auto& desc = reg.get<dispcomp::image_desc_t>(contentDispObj_);
            if (desc.ext.fill != val) {
                desc.ext.fill = val;
                reg.emplace_or_replace<dispcomp::mesh_dirty>(contentDispObj_);
            }
        }
    }

    FillOrigin GLoader::fillOrigin() const {
        if (contentDispObj_ && reg.any_of<dispcomp::image_desc_t>(contentDispObj_))
            return reg.get<dispcomp::image_desc_t>(contentDispObj_).ext.fillOrig;
        return FillOrigin::None;
    }

    void GLoader::setFillOrigin(FillOrigin val) {
        pendingFillOrigin_ = val;
        if (contentDispObj_ && reg.any_of<dispcomp::image_desc_t>(contentDispObj_)) {
            auto& desc = reg.get<dispcomp::image_desc_t>(contentDispObj_);
            if (desc.ext.fillOrig != val) {
                desc.ext.fillOrig = val;
                reg.emplace_or_replace<dispcomp::mesh_dirty>(contentDispObj_);
            }
        }
    }

    bool GLoader::fillClockwise() const {
        if (contentDispObj_ && reg.any_of<dispcomp::image_desc_t>(contentDispObj_))
            return reg.get<dispcomp::image_desc_t>(contentDispObj_).ext.fillClockwise;
        return true;
    }

    void GLoader::setFillClockwise(bool val) {
        pendingFillClockwise_ = val;
        if (contentDispObj_ && reg.any_of<dispcomp::image_desc_t>(contentDispObj_)) {
            auto& desc = reg.get<dispcomp::image_desc_t>(contentDispObj_);
            if (desc.ext.fillClockwise != val) {
                desc.ext.fillClockwise = val;
                reg.emplace_or_replace<dispcomp::mesh_dirty>(contentDispObj_);
            }
        }
    }

    float GLoader::fillAmount() const {
        if (contentDispObj_ && reg.any_of<dispcomp::image_desc_t>(contentDispObj_))
            return reg.get<dispcomp::image_desc_t>(contentDispObj_).ext.fillAmount;
        return 1.0f;
    }

    void GLoader::setFillAmount(float val) {
        pendingFillAmount_ = val;
        if (contentDispObj_ && reg.any_of<dispcomp::image_desc_t>(contentDispObj_)) {
            auto& desc = reg.get<dispcomp::image_desc_t>(contentDispObj_);
            if (desc.ext.fillAmount != val) {
                desc.ext.fillAmount = val;
                reg.emplace_or_replace<dispcomp::mesh_dirty>(contentDispObj_);
            }
        }
    }

    // ============================================================
    // Serialization (from .fui binary)
    // ============================================================

    void GLoader::setupBeforeAdd(ByteBuffer& buffer, int startPos) {
        Object::setupBeforeAdd(buffer, startPos);

        buffer.seekToBlock(startPos, ObjectBlocks::FillInfo);

        url_            = buffer.read<csref>();
        align_          = (AlignType)buffer.read<uint8_t>();
        verticalAlign_  = (VertAlignType)buffer.read<uint8_t>();
        fill_           = (LoaderFillType)buffer.read<uint8_t>();
        shrinkOnly_     = buffer.read<bool>();
        autoSize_       = buffer.read<bool>();
        showErrorSign_  = buffer.read<bool>();
        playing_        = buffer.read<bool>();
        frame_          = buffer.read<int>();

        if (buffer.read<bool>())
            color_ = buffer.read<Color4B>();

        // Fill method — store as pending; applied when content is created.
        pendingFillMethod_ = (FillMethod)buffer.read<uint8_t>();
        if (pendingFillMethod_ != FillMethod::None) {
            pendingFillOrigin_    = (FillOrigin)buffer.read<uint8_t>();
            pendingFillClockwise_ = buffer.read<bool>();
            pendingFillAmount_    = buffer.read<float>();
        }

        if (buffer.version >= 7)
            useResize_ = buffer.read<bool>();

        if (!url_.empty())
            loadContent();
    }

    void GLoader::setupAfterAdd(ByteBuffer& buffer, int startPos) {
        Object::setupAfterAdd(buffer, startPos);
    }

}
