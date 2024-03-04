#include "package_item.h"
#include <core/declare.h>
#include "core/package.h"


namespace gui {

    PackageItem::PackageItem()
        : owner_(nullptr)
        , type_(PackageItemType::Component)
        , objType_(ObjectType::Component)
        , id_()
        , name_()
        , file_()
        , width_(0)
        , height_(0)
        , rawData_()
        , branches_()
        , highResolution_()
        , texture_(nullptr)
        , tileGridIndex_(0)
        , scaledByTile_(false)
        , interval_(0.0f)
        , repeatDelay_(0.0f)
        , swing_(false)
        , extensionCreator_()
        , translated_(false)
        , skeletonAnchor_{}
    {}

    PackageItem::~PackageItem() {
        if(scale9Grid_) {
            delete scale9Grid_;
        }
        // if(rawData_) {
        //     delete rawData_;
        // }
        if(branches_) {
            delete branches_;
        }
        if(highResolution_) {
            delete highResolution_;
        }
    }

    PackageItem* PackageItem::getBranch(){
        do {
            if(!branches_->size()) {
                break;
            }
            if(owner_->branchIndex() == -1) {
                break;
            }
            auto const& itemID = (*branches_)[owner_->branchIndex()];
            if(!itemID.size()) {
                break;
            }
            return owner_->itemByID(itemID);
        }while(true);
        return nullptr;
    }

    PackageItem const* PackageItem::getBranch() const{
        do {
            if(!branches_->size()) {
                break;
            }
            if(owner_->branchIndex() == -1) {
                break;
            }
            auto const& itemID = (*branches_)[owner_->branchIndex()];
            if(!itemID.size()) {
                break;
            }
            return owner_->itemByID(itemID);
        }while(true);
        return nullptr;
    }
}