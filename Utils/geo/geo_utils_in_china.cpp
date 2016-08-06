#ifdef _WIN32
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#endif

#define _USE_MATH_DEFINES
#include <math.h>
#include "geo_utils.h"


#define MIN(x, y)   (x) < (y) ? (x) : (y)
#define MAX(x, y)   (x) > (y) ? (x) : (y)
#define COUNT_OF(a) (sizeof(a)/sizeof(*(a)))

using namespace std;

namespace geo {

// refer to http://www.cnblogs.com/Aimeast/archive/2012/08/09/2629614.html

class Rectangle
{
public:
    double West, North, East, South;

    Rectangle(double latitude1, double longitude1, double latitude2, double longitude2)
    {
        this->West = MIN(longitude1, longitude2);
        this->North = MAX(latitude1, latitude2);
        this->East = MAX(longitude1, longitude2);
        this->South = MIN(latitude1, latitude2);
    }

};

static const Rectangle *region[] =
{
    new Rectangle(49.220400, 79.446200, 42.889900, 96.330000),
    new Rectangle(54.141500, 109.687200, 39.374200, 135.000200),
    new Rectangle(42.889900, 73.124600, 29.529700, 124.143255),
    new Rectangle(29.529700, 82.968400, 26.718600, 97.035200),
    new Rectangle(29.529700, 97.025300, 20.414096, 124.367395),
    new Rectangle(20.414096, 107.975793, 17.871542, 111.744104),
};

static const Rectangle *exclude[] =
{
    new Rectangle(25.398623, 119.921265, 21.785006, 122.497559),
    new Rectangle(22.284000, 101.865200, 20.098800, 106.665000),
    new Rectangle(21.542200, 106.452500, 20.487800, 108.051000),
    new Rectangle(55.817500, 109.032300, 50.325700, 119.127000),
    new Rectangle(55.817500, 127.456800, 49.557400, 137.022700),
    new Rectangle(44.892200, 131.266200, 42.569200, 137.022700),
};


static bool InRectangle(const Rectangle &rect, double lat, double lng)
{
    return rect.West <= lng && rect.East >= lng && rect.North >= lat && rect.South <= lat;
}

bool is_inside_china(double lat, double lng)
{
    for (int i = 0; i < COUNT_OF(region); i++) {
        if (InRectangle(*region[i], lat, lng)) {
            for (int j = 0; j < COUNT_OF(exclude); j++) {
                if (InRectangle(*exclude[j], lat, lng)) {
                    return false;
                }
            }
            return true;
        }
    }
    return false;
}


}
