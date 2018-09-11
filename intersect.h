#ifndef MI_INTERSECT_H
#define MI_INTERSECT_H

#include "globals.h"

#include <vector>

namespace mesh {
    myfloat intersection_volume(const std::vector<triangle> &first_mesh, const std::vector<triangle> &second_mesh);
    myfloat intersection_volume(const std::vector<ntriangle> &first_mesh, const std::vector<ntriangle> &second_mesh);
}

#endif
