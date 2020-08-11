#include "name.h"

namespace ugi {

    Name::Name( const std::string* str )
        :_str(str) {
    }

    Name::Name( Name&& name ) noexcept {
        _str = name._str;
        name._str = nullptr;
    }

    Name::Name( const Name& name ) {
        _str = name._str;
    }

    Name& Name::operator=( Name&& name ) noexcept {
        _str = name._str;
        name._str = nullptr;
        return *this;
    }

    Name& Name::operator=( const Name& name ) {
        _str = name._str;
        return *this;
    }

    const std::string& Name::string() const {
        if(_str){
            return *_str;
        } else {
            static std::string emptyString("");
            return emptyString;
        }
    }
    const char* Name::c_str() const {
        if(_str) {
            return _str->c_str();
        } else {
            return "";
        }
    }
    size_t Name::length() const {
        if(_str) {
            return _str->length();
        } else {
            return 0;
        }
    }

    Name NamePool::getName( const char* str ) {
        std::string s(str);
        auto p = _stringTable.insert(std::move(s));
        return Name(&(*p.first));
    }
    Name NamePool::getName( const std::string& str ) {
        auto p = _stringTable.insert(str);
        return Name(&(*p.first));
    }
    Name NamePool::getName( std::string&& str ) {
        auto p = _stringTable.insert(std::move(str));
        return Name(&(*p.first));
    }
}