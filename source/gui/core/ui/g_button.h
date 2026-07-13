#pragma once
#include <core/ui/component.h>
#include <core/controller.h>

namespace gui {

    enum class ButtonMode : uint8_t {
        Common, Check, Radio
    };

    class GButton : public Component {
    public:
        static constexpr const char* UP               = "up";
        static constexpr const char* DOWN             = "down";
        static constexpr const char* OVER             = "over";
        static constexpr const char* SELECTED_OVER    = "selectedOver";
        static constexpr const char* DISABLED          = "disabled";
        static constexpr const char* SELECTED_DISABLED = "selectedDisabled";

    private:
        ButtonMode      mode_               = ButtonMode::Common;
        bool            selected_           = false;
        bool            down_               = false;
        bool            over_               = false;
        bool            changeStateOnClick_ = true;

        std::string     title_;
        std::string     icon_;
        std::string     selectedTitle_;
        std::string     selectedIcon_;
        Object*         titleObject_        = nullptr;
        Object*         iconObject_         = nullptr;

        Controller*     relatedController_  = nullptr;
        std::string     relatedPageId_;

        Controller*     buttonController_   = nullptr;

    public:
        void setState(std::string const& val);

    private:
        void updateState();

    public:
        GButton();
        ~GButton();

        virtual void constructFromResource() override;
        virtual void constructExtension(ByteBuffer& buff) override;
        virtual void setupAfterAdd(ByteBuffer& buffer, int startPos) override;

        virtual void onControllerChanged(Controller* ctl) override;

        bool selected() const { return selected_; }
        void setSelected(bool val);

        ButtonMode mode() const { return mode_; }
        void setMode(ButtonMode val);

        std::string const& title() const { return title_; }
        void setTitle(std::string const& val);

        std::string const& icon() const { return icon_; }
        void setIcon(std::string const& val);

        void setTitleObject(Object* obj) { titleObject_ = obj; }
        void setIconObject(Object* obj)  { iconObject_ = obj; }

        // 供外部调用的点击模拟
        void onTouchBegin();
        void onTouchEnd();
    };

}
