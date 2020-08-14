#include <cstdlib>

namespace ugi {

    namespace ffm {

        struct Range {
            size_t id;
            size_t offset;
            size_t size;
        };

        class RangeAllocator {
        private:
        public:
            RangeAllocator( size_t  ) {
            }
        };

    }

}