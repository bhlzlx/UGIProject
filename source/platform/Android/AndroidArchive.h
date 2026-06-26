#pragma once
#include <android/asset_manager.h>
#include <android/log.h>
#include <io/archive.h>
#include <string>
#include <cstdio>
#include <cstring>
#include <vector>

namespace comm {

    class AndroidArchive : public IArchive {
    private:
        AAssetManager* _mgr;
        std::string    _root;
        std::string    _externalPath;
    public:
        AndroidArchive(AAssetManager* mgr, const std::string& external = "")
            : _mgr(mgr), _externalPath(external) {
            if (!_externalPath.empty() && _externalPath.back() != '/')
                _externalPath += '/';
        }

        IStream* openIStream(const std::string& path, BitFlags<ReadFlag>) override {
            std::string clean = (path[0] == '/') ? path.substr(1) : path;
            for (auto& c : clean) if (c == '\\') c = '/';

            // 1) APK assets 原路径
            AAsset* a = AAssetManager_open(_mgr, (_root + clean).c_str(), AASSET_MODE_STREAMING);
            if (a) return new AssetStream(a);

            // 2) APK assets 去首级目录
            auto slash = clean.find('/');
            if (slash != std::string::npos) {
                a = AAssetManager_open(_mgr, (_root + clean.substr(slash + 1)).c_str(), AASSET_MODE_STREAMING);
                if (a) return new AssetStream(a);
            }

            // 3) 外部存储 /sdcard/Android/data/{pkg}/files/
            if (!_externalPath.empty()) {
                std::string ext = _externalPath + clean;
                FILE* f = fopen(ext.c_str(), "rb");
                if (f) return new FileStream(f);
            }
            return nullptr;
        }

        OStream* openOStream(const std::string&, BitFlags<WriteFlag>) override { return nullptr; }
        bool testExist(const std::string& path) override {
            auto s = openIStream(path, {});
            if (s) { s->close(); return true; }
            return false;
        }
        bool supportListFeature() const override { return false; }
        std::vector<FileEntity> listFiles(const std::string&) override { return {}; }
        std::string rootPath() override { return _root; }
        bool readonly() const override { return true; }
        void destroy() override { delete this; }

    private:
        class AssetStream : public IStream {
            AAsset* _a;
        public:
            AssetStream(AAsset* a) : _a(a) {}
            ~AssetStream() { if (_a) AAsset_close(_a); }
            int64_t size() const override { return (int64_t)AAsset_getLength64(_a); }
            int64_t read(void* b, int64_t sz) override { return (int64_t)AAsset_read(_a, b, (size_t)sz); }
            int64_t seek(SeekOption opt, int off) override { return (int64_t)AAsset_seek64(_a, off, opt==SeekOption::begin?SEEK_SET:SEEK_CUR); }
            int64_t tell() const override { return size() - (int64_t)AAsset_getRemainingLength64(_a); }
            bool seekable() const override { return true; }
            void close() override { if (_a) { AAsset_close(_a); _a = nullptr; } }
        };

        class FileStream : public IStream {
            FILE* _f;
            int64_t _size;
        public:
            FileStream(FILE* f) : _f(f) {
                fseek(_f, 0, SEEK_END); _size = ftell(_f); fseek(_f, 0, SEEK_SET);
            }
            ~FileStream() { if (_f) fclose(_f); }
            int64_t size() const override { return _size; }
            int64_t read(void* b, int64_t sz) override { return (int64_t)fread(b, 1, (size_t)sz, _f); }
            int64_t seek(SeekOption opt, int off) override { return fseek(_f, off, opt==SeekOption::begin?SEEK_SET:SEEK_CUR); }
            int64_t tell() const override { return ftell(_f); }
            bool seekable() const override { return true; }
            void close() override { if (_f) { fclose(_f); _f = nullptr; } }
        };
    };

    inline IArchive* CreateAndroidArchive(AAssetManager* mgr, const std::string& externalPath = "") {
        return new AndroidArchive(mgr, externalPath);
    }
}
