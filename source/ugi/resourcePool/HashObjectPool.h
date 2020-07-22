#pragma once

namespace ugi {

    template< class Prototype__, class TargetType__, class HostType__, class HashMethod__, class CreateMethod__, class DestroyMethod__ >
    class HashObjectPool {
    private:
        std::unordered_map< uint64_t, TargetType__> _valueTable;
        std::unordered_map< uint64_t, Prototype__>  _prototypeTable;
        //
        HashObjectPool()
            : _valueTable {}
            , _prototypeTable{}
        {
        }

    public:
        TargetType__ getObject( const Prototype__& prototype, HostType__ host, uint64_t& hashValue ) {
            HashMethod__ hashMethod;
            uint64_t hashVal = hashMethod(prototype);
            auto iter = _valueTable.find(hashVal);
            if(  iter!= _valueTable.end()) {
                hashValue = hashVal;
                return iter->second;
            } else {
                CreateMethod__ creator;
                TargetType__ result = creator( host, prototype);
                _valueTable[hashVal] = result;
                _prototypeTable[hashVal] = prototype;
                hashValue = hashVal;
                return result;
            }
        }

        TargetType__ getObject( uint64_t hashValue ) {
            auto iter = _valueTable.find(hashValue);
            if(  iter!= _valueTable.end()) {
                return iter->second;
            }
            return TargetType__();
        }

        const Prototype__& getPrototype( uint64_t hashValue ) {
            auto iter = _prototypeTable.find(hashVal);
            if(  iter!= _prototypeTable.end()) {
                hashValue = hashVal;
                return iter->second;
            }
            return Prototype__();
        }

        void cleanup( HostType__ host ) {
            DestroyMethod__ destroyer;
            for( auto& res : _valueTable ) {
                destroyer( host, res );
            }
        }

        static HashObjectPool* GetInstance() {
            static HashObjectPool<Prototype__, TargetType__, HostType__, HashMethod__, CreateMethod__, DestroyMethod__> Pool;
            return &Pool;
        }
    };

}