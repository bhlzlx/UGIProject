/**
 * @file handle.h
 * @author bhlzlx@hotmail.com
 * @brief weak ref for ui object
 * @version 0.1
 * @date 2024-01-31
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#pragma once

#include <atomic>
#include <memory>
#include <functional>

/**
 * @brief 
 *  a handle ref to a weak pointer
 */

namespace gui {

    class RefInfo {
    public:
        using deletor = std::function<void(void*)>;
    private:
        std::atomic<int>                ref_;
        void*                           ptr_; // data
        deletor                         dtor_;
    public:
        RefInfo(void* ptr, decltype(dtor_) dtor)
            : ref_(1)
            , dtor_(dtor)
        {}

        void addRef() {
            ++ref_;
        }
        void deRef() {
            --ref_;
            if(ref_ == 0) {
                if(ptr_ && dtor_) {
                    dtor_(ptr_);
                }
            }
        }
        int refCount() const {
            return ref_;
        }
        void reset() {
            ptr_ = nullptr;
            dtor_ = nullptr;
        }
        void*  data() const {
            return ptr_;
        }
    };

    class Handle {
        friend class ObjectHandle;
    private:
        RefInfo*    ref_;
    private:
        // 逻辑层不可手动创建Handle对象，只能通过GObject来获取弱引用
        Handle(void* ptr, RefInfo::deletor dtor) {
            ref_ = new RefInfo(ptr, dtor);
        }
    public:
        Handle()
            : ref_(nullptr)
        {}
        Handle(Handle const& handle) {
            ref_ = handle.ref_;
            ref_->addRef();
        }
        Handle(Handle && handle) {
            ref_->deRef();
            ref_ = handle.ref_;
            handle.ref_ = nullptr;
        }
        template<class T>
        T* as() const {
            if(!ref_) {
                return nullptr;
            }
            return (T*)ref_->data();
        }
        operator bool () const {
            return !!as<void*>();
        }
        bool operator == (nullptr_t) const {
            return !as<void*>();
        }
        ~Handle() {
            if(ref_) {
                ref_->deRef();
            }
        }
    };

    class ObjectHandle {
    private:
        Handle handle_;
    public:
        ObjectHandle(void* ptr = nullptr, RefInfo::deletor dtor = nullptr) 
            :handle_(ptr, dtor)
        {
        }
        ~ObjectHandle() {
            handle_.ref_->reset();
        }
        Handle handle() const {
            return handle_;
        }
        void reset() {
            handle_.ref_->reset();
        }
    };

}