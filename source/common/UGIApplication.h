#pragma once
#include <cstdint>
#include <LightWeightCommon/lightweight_comm.h>

class UGIApplication {
public:
    enum eKeyEvent
    {
        eKeyDown,
        eKeyUp
    };
    enum eMouseButton
    {
        MouseButtonNone,
        LButtonMouse,
        MButtonMouse,
        RButtonMouse,
    };
    enum eMouseEvent
    {
        MouseMove,
        MouseDown,
        MouseUp
    };
    virtual bool initialize(void* _wnd, comm::IArchive*) = 0;
    virtual void resize(uint32_t _width, uint32_t _height) = 0;
    virtual void release() = 0;
    virtual void tick() = 0;
    virtual const char * title() = 0;
    virtual uint32_t rendererType() = 0;
    virtual void pause() {
        
    }
    virtual void resume( void* _wnd, uint32_t _width, uint32_t _height ) {
    }

    virtual void onKeyEvent(unsigned char _key, eKeyEvent _event) {
    }
    virtual void onMouseEvent(eMouseButton _bt, eMouseEvent _event, int _x, int _y) {
    }
};

UGIApplication * GetApplication();