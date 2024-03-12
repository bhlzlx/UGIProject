#include "package.h"
#include "core/declare.h"
#include "core/n_texture.h"
#include "core/package.h"
#include "core/ui/object_factory.h"
#include "ugi_types.h"
#include <utils/byte_buffer.h>
#include <utils/toolset.h>
//
#include <ugi/device.h>
#include <ugi/render_context.h>
#include <ugi/texture.h>
#include <ugi/texture_util.h>
#include <ugi/command_buffer.h>
//
#include <io/archive.h>
#include <log/client_log.h>
//
#include <core/package_item.h>
#include <core/ui/object.h>

namespace gui {

    comm::IArchive*                                 Package::archive_ = nullptr;
    std::unordered_map<std::string, Package*>       Package::packageInstByID;
    std::unordered_map<std::string, Package*>       Package::packageInstByName;
    std::vector<Package*>                           Package::packageList_;
    std::unordered_map<std::string, std::string>    Package::vars_;
    std::string                                     Package::branch_;
    Texture*                                        Package::emptyTexture_;
    uint32_t                                        Package::moduleInited_;

    namespace {
        uint32_t White2x2Data[] = {
            0xffffffff,0xffffffff,
            0xffffffff,0xffffffff,
        };
    }


    Package::Package() {
    }
    Package::~Package() {
    }

    bool Package::CheckModuleInitialized() {
        if(moduleInited_ && archive_) {
            return true;
        }
        return false;
    }

    bool Package::loadFromBuffer(ByteBuffer& bufferRef, std::string_view assetPath) {
        auto buffer = &bufferRef;
        auto magicNumber = buffer->read<uint32_t>();
        // magic number, version, bool, id, name,[20 bytes], indexTables
        if(magicNumber != 0x46475549) {
            return false;
        }
        assetPath_ = assetPath;
        buffer->version = buffer->read<int>();
        bool v2 = buffer->version >= 2;
        buffer->read<bool>();
        id_ = buffer->read<std::string>();
        name_ = buffer->read<std::string>();
        buffer->skip(20);
        // index tables
        int indexTablePos = buffer->pos();
        int count = 0;
        // read string table
        buffer->seekToBlock(indexTablePos, PackageBlocks::StringTable); {
            count = buffer->read<int>();
            stringTable_.resize(count);
            for(int i = 0; i<count; ++i) {
                stringTable_[i] = buffer->read<std::string>();
            } 
        }
        buffer->setStringTable(&stringTable_);
        // read dependences
        buffer->seekToBlock(indexTablePos, PackageBlocks::Dependences); {
            count = buffer->read<int16_t>();
            for(int i = 0; i<count; ++i) {
                auto const& id = buffer->read<csref>();
                auto const& name = buffer->read<csref>();
                dependencies_.push_back({id, name});
            }
        }
        // read branch info
        bool branchIncluded = false;
        if(v2) {
            count = buffer->read<int16_t>();
            if(count>0) {
                buffer->readRefStringArray(branches_, count);
                branchIndex_ = gui::toolset::findIndexStringArray(branches_, branch_);
            }
            branchIncluded = count > 0;
        }
        // read package items
        buffer->seekToBlock(indexTablePos, PackageBlocks::Items);
        PackageItem* item = nullptr;
        auto slashPos = assetPath_.find('/');
        std::string basePath;
        if(slashPos != std::string::npos) {
            basePath = std::string(assetPath_.begin(), assetPath_.begin()+slashPos+1);
        }
        count = buffer->read<int16_t>();
        for(int i = 0; i<count; ++i) {
            int nextPos = buffer->read<int>();
            nextPos += buffer->pos();
            item = new PackageItem();
            item->owner_ = this;
            item->type_ = (PackageItemType)buffer->read<uint8_t>();
            item->id_ = buffer->read<csref>();
            item->name_ = buffer->read<csref>();
            buffer->skip(2); // ???? what's up!
            item->file_ = buffer->read<csref>();
            buffer->read<bool>(); // no use!
            item->width_ = buffer->read<int>();
            item->height_ = buffer->read<int>();
            //
            switch(item->type_) {
                case PackageItemType::Image: {
                    item->objType_ = ObjectType::Image;
                    ImageScaleOption scaleOption = (ImageScaleOption)buffer->read<uint8_t>();
                    if(ImageScaleOption::Grid9 == scaleOption) {
                        item->scale9Grid_ = new gui::Rect<float>();
                        item->scale9Grid_->base.x = buffer->read<int>();
                        item->scale9Grid_->base.y = buffer->read<int>();
                        item->scale9Grid_->size.width = buffer->read<int>();
                        item->scale9Grid_->size.height = buffer->read<int>();
                    } else if(ImageScaleOption::Tile == scaleOption) {
                        item->scaledByTile_ = true;
                    }
                    buffer->read<bool>(); // smoothing
                    break;
                }
                case PackageItemType::MovieClip: {
                    buffer->read<bool>(); // smoothing
                    item->objType_ = ObjectType::MovieClip;
                    item->rawData_ = buffer->read<ByteBuffer>().clone();  // a slice of byte buffer
                    break;
                }
                case PackageItemType::Font: {
                    item->rawData_ = buffer->read<ByteBuffer>().clone();
                    break;
                }
                case PackageItemType::Component: {
                    uint32_t ext = buffer->read<uint8_t>();
                    if(ext) {
                        item->objType_ = (ObjectType)ext;
                    } else {
                        item->objType_ = ObjectType::Component;
                    }
                    item->rawData_ = buffer->read<ByteBuffer>().clone();
                    // todo: UIObjectFactory::Re...
                    break;
                }
                case PackageItemType::Atlas:
                case PackageItemType::Sound:
                case PackageItemType::Misc: {
                    item->file_ = assetPath_ + "_" + item->file_;
                    break;
                }
                case PackageItemType::DragonBones:
                case PackageItemType::Spine: {
                    item->file_ = basePath + item->file_;
                    item->skeletonAnchor_.x = buffer->read<float>();
                    item->skeletonAnchor_.y = buffer->read<float>();
                    break;
                }
                default: {
                    break;
                }
            }
            //
            if(v2) { // v2 之后有更多的属于
                std::string str = buffer->read<csref>(); // 这是分枝信息
                if(!str.empty()) {
                    item->name_ = str + "/" + item->name_;
                }
                auto branchCount = buffer->read<uint8_t>();
                if(branchCount) {
                    if(branchIncluded) {
                        item->branches_ = new std::vector<std::string>();
                        buffer->readRefStringArray(*item->branches_, branchCount);
                    } else {
                        auto const& key = buffer->read<csref>();
                        itemsByID_[key] = item;
                    }
                }
                auto highResCount = buffer->read<uint8_t>();
                if(highResCount) {
                    item->highResolution_ = new std::vector<std::string>();
                    buffer->readRefStringArray(*item->highResolution_, highResCount);
                }
            }
            packageItems_.push_back(item);
            itemsByID_[item->id_] = item;
            if(!item->name_.empty()) {
                itemsByName_[item->name_] = item;
            }
            buffer->setPos(nextPos);
        }
        // read spirites
        buffer->seekToBlock(indexTablePos, PackageBlocks::Sprites);
        count = buffer->read<int16_t>();
        for(int i = 0; i<count; ++i) {
            int nextPos = buffer->read<uint16_t>();
            nextPos += buffer->pos();
            auto const& itemID = buffer->read<csref>();
            item = itemsByID_[buffer->read<csref>()];
            //
            AtlasSprite sprite;
            sprite.item = item;
            sprite.rect.base.x = buffer->read<int>();
            sprite.rect.base.y = buffer->read<int>();
            sprite.rect.size.width = buffer->read<int>();
            sprite.rect.size.height = buffer->read<int>();
            sprite.rotated = buffer->read<bool>();
            if(v2 && buffer->read<bool>()) {
                sprite.offset.x = buffer->read<int>();
                sprite.offset.y = buffer->read<int>();
                sprite.origSize.x = buffer->read<int>();
                sprite.origSize.y = buffer->read<int>();
            } else {
                sprite.offset = {};
                sprite.origSize = sprite.rect.size;
            }
            sprites_[itemID] = sprite;
            buffer->setPos(nextPos);
        }
        // pixel hit test data
        if(buffer->seekToBlock(indexTablePos, PackageBlocks::HitTestData)) {
            count = buffer->read<int16_t>();
            for(int i = 0; i<count; ++i) {
                int nextPos = buffer->read<int>();
                nextPos += buffer->pos();
                //
                auto const& id = buffer->read<csref>();
                auto iter = itemsByID_.find(id);
                if(iter != itemsByID_.end()) {
                    item = iter->second;
                    if(item->type_ == PackageItemType::Image) {
                        item->pixelHitTestData_ = new PixelHitTestData();
                        item->pixelHitTestData_->load(*buffer);
                    }
                }
                buffer->setPos(nextPos);
            }
        }
        return true;
    }

    Package* Package::AddPackage(std::string const& assetPath) {
        // if(!CheckModuleInitialized()) {
        //     assert("not initialized yet!");
        //     return nullptr;
        // }
        auto iter = packageInstByID.find(assetPath);
        if(iter != packageInstByID.end()) {
            return iter->second;
        }
        auto file = archive_->openIStream(assetPath + "_fui.bytes", {comm::ReadFlag::binary});
        if(!file) {
            COMMLOGE("GUI: package not found [%s]", assetPath.c_str());
            return nullptr;
        }
        ByteBuffer buff(file->size());
        file->read(buff.ptr(), file->size());
        // ready to read, create a package object
        Package* package = new Package();
        package->assetPath_ = assetPath;
        auto rst = package->loadFromBuffer(buff, assetPath);
        if(!rst) {
            delete package;
            return nullptr;
        }
        packageInstByID[package->id_] = package;
        packageInstByID[assetPath] = package;
        packageInstByName[package->name_] = package;
        packageList_.push_back(package);
        return package;
    }

    void Package::InitPackageModule(comm::IArchive* archive) {
        if(!archive) {
            return ;
        }
        archive_ = archive;
        auto renderContext = ugi::StandardRenderContext::Instance();
        auto device = renderContext->device();
        // init empty textur
        ugi::tex_desc_t whiteTexDesc = {
            .type = ugi::TextureType::Texture2D,
            .format = ugi::UGIFormat::RGBA8888_UNORM,
            .mipmapLevel = 1,
            .layerCount = 1,
            .width = 2,
            .height = 2,
            .depth = 1,
        };
        ugi::image_region_t region = {
            .mipLevel = 0,
            .arrayIndex = 0,
            .arrayCount = 1,
            .offset = {0, 0, 0},
            .extent = {2, 2, 1},
        };
        emptyTexture_ = device->createTexture(whiteTexDesc);
        uint64_t offset = 0;
        if(emptyTexture_) {
            emptyTexture_->updateRegions(device, &region, 1, (uint8_t const*)White2x2Data, sizeof(White2x2Data), &offset, renderContext->asyncLoadManager(),[](void* res, ugi::CommandBuffer*) {
                // async load complete!
                // do nothing
            });
        }
    }

    Object* Package::createObject(std::string const& resName) {
        auto iter = itemsByName_.find(resName);
        if(iter == itemsByName_.end()) {
            return nullptr;
        }
        auto obj = ObjectFactory::CreateObject(iter->second);
        if(!obj) {
            return nullptr;
        }
        ++constructing_;
        obj->constructFromResource();
        --constructing_;
        return obj;
    }

    PackageItem* Package::itemByID(std::string const& id) {
        auto iter = itemsByID_.find(id);
        if(iter != itemsByID_.end()) {
            return iter->second;
        }
        return nullptr;
    }

    void Package::loadAllAssets() {
        for(auto [id, item]: itemsByID_) {
            loadAssetItem(item);
        }
    }

    void Package::loadAssetItem(PackageItem* item) {
        switch(item->type_) {
            case gui::PackageItemType::Atlas: {
                loadAtlasItem(item);
                break;
            }
            case gui::PackageItemType::Image: {
                loadImageItem(item);
                break;
            }
            default:
            break;
        }
    }

    void Package::loadAtlasItem(PackageItem* item) {
        if(item->rawTexture_) { // 已经加载过了
            return;
        }
        auto const& filepath = item->file_;
        auto rc = ugi::StandardRenderContext::Instance();
        auto file = archive_->openIStream(filepath, {comm::ReadFlag::binary});
        ugi::Texture* tex = nullptr;
        if(file) {
            std::vector<uint8_t> pngData;
            pngData.resize(file->size());
            file->read(pngData.data(), file->size());
            tex = rc->createTexturePNG(pngData.data(), pngData.size(), [](void* res, ugi::CommandBuffer* cmd) {
                // async load
            } );
        }
        if(!tex) {
            tex = emptyTexture_;
        }
        if(item->rawTexture_ == nullptr) {
            item->rawTexture_ = tex;
        // } else {
        //     assert(false); // 尚未处理
        }
        if(item->rawTexture_) {
            item->texture_ = NTexture(item->rawTexture_->handle(), Rect<float>{{0, 0}, {(float)tex->desc().width, (float)tex->desc().height}});
        }
    }

    void Package::loadImageItem(PackageItem* item) {
        auto iter = sprites_.find(item->id_);
        if(iter != sprites_.end()) {
            auto const& sprite = iter->second;
            if(nullptr == sprite.item->rawTexture_) {
                loadAtlasItem(sprite.item);
            }
            auto atlas = sprite.item->texture_;
            if(atlas.size().width == sprite.rect.size.width && atlas.size().height == sprite.rect.size.height) {
                item->texture_ = atlas;
            } else {
                item->texture_ = NTexture(atlas, sprite.rect, sprite.rotated, Size2D<float>(sprite.origSize.x, sprite.origSize.y), sprite.offset);
            }
        }
    }

    // bool Package::CheckModuleInitialized() {
    //     return true;
    // }


    Package* PackageForID(std::string const& id) {
        return nullptr;
    }

    Package* PackageForName(std::string const& name) {
        return nullptr;
    }

    Package* LoadPackageFromAsset(std::string const& assetPath) {
        return nullptr;
    }


}