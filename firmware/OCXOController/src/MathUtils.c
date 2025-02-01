#include "MathUtils.h"

double lerp(double x0, double y0, double x1, double y1, double x) {
    return y1 - (x1 - x)*(y1 - y0)/(x1 - x0);
}