#pragma once
#include <string>
#include <set>

namespace ugi {

    class Name {
    private:
        const std::string* _str;
    public:
        Name( const std::string* str = nullptr );
        Name( Name&& name ) noexcept;
        Name( const Name& name );
        Name& operator=( Name&& name ) noexcept;
        Name& operator=( const Name& name );

        const std::string& string() const;
        const char* c_str() const ;
        size_t length() const ;
    };

    class NamePool {
    private:
        struct NameLess {
            bool operator () (const std::string& a, const std::string& b) const {
                auto iter1 = a.rbegin();
                auto iter2 = b.rbegin();
                while(true) {
                    char cha, chb = 0;
                    uint8_t flag = 0x0;
                    if(iter1 != a.rend()) {
                        cha = *iter1;
                        flag|=0x1;
                        ++iter1;
                    }
                    if(iter2 != b.rend()) {
                        chb = *iter2;
                        flag|=0x2;
                        ++iter2;
                    }
                    if(cha!=chb) {
                        return cha < chb;
                    }
                    if(!flag) {
                        return false;
                    }
                }
                return false;
            }
        };
        std::set< std::string, NameLess > _stringTable;
    public:
        NamePool() {
        }
        Name getName( const char* str );
        Name getName( const std::string& str );
        Name getName( std::string&& str );
        //
        ~NamePool() {
        }
    };

}

