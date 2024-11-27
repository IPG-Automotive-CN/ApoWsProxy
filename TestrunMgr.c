#include "InfoUtils.h"
#include "TestrunMgr.h"
#include <stdlib.h>
#include <string.h>

#include <road.h>

TestrunInfo tsInfo;

//TODO
const char *CM_Install_Dir = "/opt/ipg/carmaker/linux64-13.1/";

char *xodr_path = NULL;
double egoOffsetZ = 0.0;

static tInfos* readInfoFile(const char *path, const char *prefix)
{
    tErrorMsg *err;
    tInfos *inf = InfoNew();
    int ret = iRead2(&err, inf, path, prefix);
    if(ret != 0)
    {
        //printf("Error reading InfoFile file: %s\n", path);
        return NULL;
    }
    return inf;
}

char* getOpenDrivePath()
{
    return xodr_path;
}


int WriteOpenDrive(tInfos *inf, const char* dir)
{
    tRoad *road = RoadNew();
    char *roadFName = iGetStrOpt(inf, "Road.FName", "");
    if(strcmp(roadFName, "") != 0)
    {
        char rd5Path[1024];
        snprintf(rd5Path, sizeof(rd5Path), "%s/Data/Road/%s", dir, roadFName);
        printf("rd5Path = %s\n", rd5Path);
        tInfos *inf_road = readInfoFile(rd5Path, "IPGRoad");

        if(inf_road == NULL)
        {
            //Try Again
            snprintf(rd5Path, sizeof(rd5Path), "%s/Data/Road/%s", CM_Install_Dir, roadFName);
            inf_road = readInfoFile(rd5Path, "IPGRoad");
            if(inf_road == NULL)
            {
                printf("Error reading road file: %s\n", roadFName);
                return -1;
            }
        }

        RoadReadInf(road, inf_road);
    }else
    {
        RoadReadInf(road, inf);
    }

    if(road == NULL)
    {
        printf("road is NULL\n");
        return -1;
    }
    int revNumber[2] = {1, 4};


    char outputPath[1024];
    snprintf(outputPath, sizeof(outputPath), "%s/Data/Road/temp.xodr", dir);

    printf("outputPath = %s\n", outputPath);
    int ret = RoadWriteOpenDRIVE(road, outputPath, revNumber, "OSCCommonRoad", 1);
    printf("RoadWriteOpenDRIVE return %d\n", ret);
    
    if(ret == 0)
    {
        xodr_path = (char *)malloc(strlen(outputPath)+1);
        memcpy(xodr_path, outputPath, strlen(outputPath)+1);
    }

    return ret;
}


int loadTestrun(const char* projectdir, const char *testrun)
{
    char path[1024];
    snprintf(path, sizeof(path), "%s/Data/TestRun/%s", projectdir, testrun);
    tInfos *inf = readInfoFile(path, "TestRun");
    if(inf == NULL)
    {
        sprintf(path, "%s/Data/TestRun/%s", CM_Install_Dir, testrun);
        inf = readInfoFile(path, "TestRun");
        if(inf == NULL)
        {
            printf("Error reading testrun file: %s\n", testrun);
            return -1;
        }
    }

    //TODO projectdir
    WriteOpenDrive(inf, projectdir);



    int n = iGetIntOpt(inf, "Traffic.N", 0);

    tsInfo.traffic_nObjs = n;
    tsInfo.trafficObjs = (TrafficInfo*)malloc(sizeof(TrafficInfo)*n);

    tsInfo.trafficNames = (char **)malloc(sizeof(char*)*n);


    char *egoFName = iGetStrOpt(inf, "Vehicle", "");
    char egoFPath[1024];
    tInfos *egoInf = NULL;
    if(strcmp(egoFName, "") != 0)
    {
        sprintf(egoFPath, "%s/Data/Vehicle/%s", projectdir, egoFName);
        egoInf = readInfoFile(egoFPath, "Vehicle");

        if(egoInf == NULL)
        {
            //Try Again
            sprintf(egoFPath, "%s/Data/Vehicle/%s", CM_Install_Dir, egoFName);
            egoInf = readInfoFile(egoFPath, "Vehicle");
            if(egoInf == NULL)
            {
                printf("Error reading ego file: %s\n", egoFName);
            }
        }

        if(egoInf != NULL)
        {
            int nRows = 0;
            double *pos = iGetTableOpt2 (egoInf, "Body.pos", NULL, 3, &nRows);
            if(pos != NULL && nRows == 1)
            {
                egoOffsetZ = pos[2];
                //printf("egoOffsetZ = %lf\n", egoOffsetZ);
            }
        }

    }

    for(int i = 0;i<n;i++)
    {
        char key[1024];
        snprintf(key, sizeof(key), "Traffic.%d.Name", i);

        char *name = iGetStrOpt(inf, key, "");
        tsInfo.trafficNames[i] = (char *)malloc(strlen(name)+1);
        memcpy(tsInfo.trafficNames[i], name, strlen(name)+1);

        snprintf(key, sizeof(key), "Traffic.%d.Template.FName", i);
        char *templateFPath = iGetStrOpt(inf, key, "");

        char absTemplatePath[1024];
        snprintf(absTemplatePath, sizeof(absTemplatePath), "%s/Data/Traffic/Template/%s", projectdir, templateFPath);
        tInfos *templateInf = readInfoFile(absTemplatePath, "traffic template");

        if(templateInf == NULL)
        {
            //Try Again
            snprintf(absTemplatePath, sizeof(absTemplatePath), "%s/Data/Traffic/Template/%s", CM_Install_Dir, templateFPath);
            templateInf = readInfoFile(absTemplatePath, "traffic template");
            if(templateInf == NULL)
            {
                printf("Error reading template file: %s\n", templateFPath);
                continue;
            }
        }

        char *rcsClass = iGetStrOpt(templateInf, "RCSClass", "RCSUnknown");
        if(strcmp(rcsClass, "RCS_Car") == 0)
        {
            tsInfo.trafficObjs[i].rcsClass = RCS_Car;
        }else if(strcmp(rcsClass, "RCS_Truck") == 0)
        {
            tsInfo.trafficObjs[i].rcsClass = RCS_Truck;
        }else
        {
            tsInfo.trafficObjs[i].rcsClass = RCS_Unknown;
        }

        int nRows = 0;
        double *dv = iGetTableOpt2 (templateInf, "Basics.Offset", NULL, 2, &nRows);
        if(dv != NULL && nRows == 1)
        {
            tsInfo.trafficObjs[i].offset = dv[0];
        }
        dv = iGetTableOpt2(templateInf, "Basics.Dimension", NULL, 3, &nRows);
        if(dv != NULL && nRows == 1)
        {
            tsInfo.trafficObjs[i].dimension[0] = dv[0];
            tsInfo.trafficObjs[i].dimension[1] = dv[1];
            tsInfo.trafficObjs[i].dimension[2] = dv[2];
        }
        // printf("traffic %d, name: %s, rcsClass: %s, l : %lf, w : %lf, h: %lf\n", i, 
        //     tsInfo.trafficNames[i], rcsClass, dv[0], dv[1], dv[2]);
    }

    return 0;
}


int isTestRunLoaded()
{
    return xodr_path != NULL;
}