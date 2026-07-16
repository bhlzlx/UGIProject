#pragma  once

#include "core/data_types/ui_types.h"
#include "core/declare.h"
#include "core/n_texture.h"
#include "object.h"
#include "IColorGear.h"

namespace gui {

    class Image : public Object, public IColorGear {
    private:
        glm::vec3           color_;
        std::string         icon_;
        FlipType            flip_;
        FillMethod          fillMethod_;
        FillOrigin          fillOrigin_;
        float               fillAmount_; 
        bool                clockwise_;
        NTexture            texture_;
    public:
        Image()
            : Object()
            , color_(0)
            , flip_(FlipType::None)
            , fillMethod_(FillMethod::None)
            , fillOrigin_(FillOrigin::None)
            , fillAmount_(1.0f)
            , clockwise_(true)
            , texture_()
        {
            type_ = ObjectType::Image;
        }

        ~Image() {}

        virtual void setSize(Size2D<float> const& size) override;
        glm::vec3 getColor() const override { return color_; }
        void setColor(glm::vec3 const& val) override;
        std::string const& getIcon() const { return icon_; }
        void setIcon(std::string const& val) { icon_ = val; }

        virtual void createDisplayObject() override;
        virtual void constructFromResource() override;
        virtual void setupBeforeAdd(ByteBuffer& buffer, int startPos = 0) override;
        virtual void setupAfterAdd(ByteBuffer& buffer, int startPos = 0) override;
    };

}