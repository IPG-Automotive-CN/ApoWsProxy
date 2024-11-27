#pragma once
#include <stdint.h>
typedef enum
{
    RCS_Unknown = 0,
    RCS_Car,
    RCS_Truck,
}tRCSClass;

typedef struct
{
    uint64_t rcsClass;
    //length, width, height
    double  dimension[3];
    //the z-offset defines the distance of the traffic objects bounding box from the road surface
    double offset; 
}TrafficInfo;

typedef struct 
{
    uint64_t traffic_nObjs; 
    TrafficInfo* trafficObjs;
    char **trafficNames;
}TestrunInfo;

typedef struct {
    double x;
    double y;
    double z;
    double heading;
}position;

typedef struct
{
    double time;
    position ego;
    uint64_t others_num;
    position* others;
}simStatus;