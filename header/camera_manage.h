#ifndef CAMERA_MANAGE_H
#define CAMERA_MANAGE_H

#include <QObject>
#include <QColor>
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <cmath>
#include <vector>
#include <thread>
#include <mutex>
#include <QDebug>
#include "Spinnaker.h"
#include "SpinGenApi/SpinnakerGenApi.h"
#include "camera_interface.h"

using namespace Spinnaker;
using namespace Spinnaker::GenApi;
using namespace Spinnaker::GenICam;
using namespace std;

class CamManager : public QObject
{
    Q_OBJECT
public:
    CamManager(QObject *parent = 0);
    vector<CamInterface> &getInterfacelist();
    void configCamera(QString, QString, QString, QString);
    ~CamManager();
private Q_SLOTS:
    void ListAllDeviceInfo();

Q_SIGNALS :
    void updateList();
    void sendMessage(QString, QColor);
    void sendError();
private:
    void ConfigureCamera(std::string, std::string, std::string, std::string);
    void manageList(vector<CamInterface> &, CamInterface &);
    void QueryInterface(InterfacePtr pInterface);
    bool ValidateIPV4Address(string , bool);
    bool IsDigits(const std::string& str);
    CamInfo PrintDeviceInfo(CameraPtr pCamera);
    gcstring GetDottedAddress(int64_t value);
    gcstring GetMACAddress(int64_t value);
    void AutoConfigure();
    string IPV4ToLongString(string ipAddress);
    vector<string> split(const string& str);

    //Thread
    std::thread *listcallback_thread_;
    std::mutex thread_mtx_;

    //net data string
    string config_data_[4];
    bool loop_ctrl_;
    bool config_flag;

    //Variable
    vector<CamInterface> interface_list_;
};

#endif // CAMERA_MANAGE_H
