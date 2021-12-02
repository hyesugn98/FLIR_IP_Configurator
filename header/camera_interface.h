#ifndef CAMERA_INTERFACE_H
#define CAMERA_INTERFACE_H
#include <QString>
#include <vector>

using namespace std;

typedef struct camInfo
{
    QString SerialNumber;
    QString ModelName;
    QString VendorName;
    QString IPAddress;
    QString SubnetMask;
    QString Gateway;
    QString MACAddress;

}CamInfo;

typedef struct camInterface
{
    vector<CamInfo> camera_list;
    QString interface_name;
    uint8_t interface_id;

     QString IPAddress;
}CamInterface;

#endif // CAMERA_INTERFACE_H
