#ifndef PFN_SERIALIZER_H
#define PFN_SERIALIZER_H

#include "../level_serializer.h"

namespace pn {

class PFNSerializer : public LevelSerializer {
public:
    PFNSerializer(Level& level):
        LevelSerializer(level) {}

    ~PFNSerializer();

    bool save_to(const std::string& filename);
    bool load_from(const std::string& filename);
};

}

#endif // PFN_SERIALIZER_H
