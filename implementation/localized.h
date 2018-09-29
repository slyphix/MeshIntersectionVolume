#ifndef MI_LOCALIZED_H
#define MI_LOCALIZED_H

#include "globals.h"

#include <vector>

namespace mesh {
    myfloat localized_intersection_volume(const std::vector<ntriangle> &first_mesh, const std::vector<ntriangle> &second_mesh);
}

#endif
