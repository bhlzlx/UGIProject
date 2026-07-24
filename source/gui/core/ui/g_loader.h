#pragma once

#include "core/ui/object.h"
#include "core/ui/IColorGear.h"
#include "core/n_texture.h"
#include "core/package_item.h"
#include "core/display_objects/display_object.h"
#include <core/ui/g_text_field.h>  // for AlignType, VertAlignType
#include <string>

namespace gui {

    /// <summary>
    /// FillType for GLoader — controls how the content is scaled to fit the loader area.
    /// Mirrors FairyGUI FillType.
    /// </summary>
    enum class LoaderFillType : uint8_t {
        None = 0,
        Scale = 1,
        ScaleMatchHeight = 2,
        ScaleMatchWidth = 3,
        ScaleFree = 4,
        ScaleNoBorder = 5,
    };

    /// <summary>
    /// GLoader — dynamic content loader.
    /// Loads images, movieclips, components from FairyGUI packages,
    /// or external textures at runtime.
    ///
    /// Implements IColorGear for color-tweening support.
    /// Mirror of FairyGUI-unity GLoader.
    /// </summary>
    class GLoader : public Object, public IColorGear {
    private:
        // ---- URL & Content ----
        std::string         url_;
        PackageItem*        contentItem_ = nullptr;

        // ---- Layout ----
        AlignType           align_          = AlignType::Left;
        VertAlignType       verticalAlign_  = VertAlignType::Top;
        bool                autoSize_       = false;
        LoaderFillType      fill_           = LoaderFillType::None;
        bool                shrinkOnly_     = false;
        bool                useResize_      = false;   // for component content: resize instead of scale
        bool                updatingLayout_ = false;

        // ---- Display ----
        Color4B             color_{0xFFFFFFFF};
        DisplayObject       contentDispObj_;     // child display object for Image/MovieClip
        Object*             content2_     = nullptr;  // GComponent* for Component content
        Object*             errorSign_    = nullptr;
        bool                showErrorSign_ = true;

        // ---- Fill method (applied to image content after creation) ----
        FillMethod          pendingFillMethod_   = FillMethod::None;
        FillOrigin          pendingFillOrigin_   = FillOrigin::None;
        bool                pendingFillClockwise_ = true;
        float               pendingFillAmount_    = 1.0f;

        // ---- MovieClip (stubbed — MovieClip not yet ported) ----
        bool                playing_   = true;
        int                 frame_     = 0;
        float               timeScale_ = 1.0f;

        // ---- Internal helpers ----
        void clearContent();
        void clearErrorState();
        void setErrorState();
        void updateLayout();

        void loadContent();
        void loadFromPackage(std::string const& itemURL);
        void loadExternal();

        /// Parse a FairyGUI URL ("ui://Package/Item") into (pkgID, itemName).
        static bool parseURL(std::string const& url, std::string& outPkgID, std::string& outItemName);

    protected:
        void onSizeChanged() override;

    public:
        GLoader();
        ~GLoader() override;

        // ---- Object overrides ----
        void createDisplayObject() override;
        void constructFromResource() override {}
        void setupBeforeAdd(ByteBuffer& buffer, int startPos = 0) override;
        void setupAfterAdd(ByteBuffer& buffer, int startPos = 0) override;

        // ---- URL ----
        std::string const& url() const { return url_; }
        void setURL(std::string const& val);

        /// icon 别名 — 等同于 url (FairyGUI gear 系统用到)
        std::string const& icon() const { return url_; }
        void setIcon(std::string const& val) { setURL(val); }

        // ---- Layout properties ----
        AlignType align() const { return align_; }
        void setAlign(AlignType val);

        VertAlignType verticalAlign() const { return verticalAlign_; }
        void setVerticalAlign(VertAlignType val);

        LoaderFillType fill() const { return fill_; }
        void setFill(LoaderFillType val);

        bool shrinkOnly() const { return shrinkOnly_; }
        void setShrinkOnly(bool val);

        bool autoSize() const { return autoSize_; }
        void setAutoSize(bool val);

        bool useResize() const { return useResize_; }
        void setUseResize(bool val);

        // ---- IColorGear ----
        Color4B getColor() const override { return color_; }
        void setColor(Color4B val) override;

        // ---- MovieClip controls (stubs for now) ----
        bool playing() const { return playing_; }
        void setPlaying(bool val);

        int frame() const { return frame_; }
        void setFrame(int val);

        float timeScale() const { return timeScale_; }
        void setTimeScale(float val);

        // ---- Content accessors ----
        /// Returns the embedded GComponent, or nullptr.
        Object* component() const { return content2_; }

        /// Returns the current texture (Image/MovieClip content), or nullptr.
        NTexture* texture() const;

        /// Directly set an external NTexture.
        void setTexture(NTexture* tex);

        // ---- Fill method delegation (Image content only) ----
        FillMethod fillMethod() const;
        void setFillMethod(FillMethod val);
        FillOrigin fillOrigin() const;
        void setFillOrigin(FillOrigin val);
        bool fillClockwise() const;
        void setFillClockwise(bool val);
        float fillAmount() const;
        void setFillAmount(float val);

        // ---- Error sign ----
        bool showErrorSign() const { return showErrorSign_; }
        void setShowErrorSign(bool val) { showErrorSign_ = val; }
    };

}
