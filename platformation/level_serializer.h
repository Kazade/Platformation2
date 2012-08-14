#ifndef LEVEL_SERIALIZER_H
#define LEVEL_SERIALIZER_H

#include <tr1/memory>
#include "level.h"

namespace pn {

class LevelSerializer {
public:
    typedef std::tr1::shared_ptr<LevelSerializer> ptr;

    LevelSerializer(Level& level):
        level_(level) {}

    virtual ~LevelSerializer() {}

    virtual bool save_to(const std::string& filename) = 0;
    virtual bool load_from(const std::string& filename) = 0;

protected:
    Level& level() { return level_; }

private:
    Level& level_;
};

}

#endif // LEVEL_SERIALIZER_H
