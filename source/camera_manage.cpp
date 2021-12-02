#include "header/camera_manage.h"

const std::string zeroSerial = "0";
const std::string zeroIPAddress = "0.0.0.0";
const std::string broadcastAddress = "255.255.255.255";

CamManager::CamManager(QObject *parent):
    QObject(parent),
    loop_ctrl_(false)
{
    listcallback_thread_ = new std::thread(&CamManager::ListAllDeviceInfo, this);
    loop_ctrl_ = true;
    config_flag = false;
}

CamManager::~CamManager() {
    loop_ctrl_ = false;
    if(!loop_ctrl_)
        listcallback_thread_->join();
    delete listcallback_thread_;
}

vector<CamInterface> &CamManager::getInterfacelist()
{
    return interface_list_;
}

void CamManager::manageList(vector<CamInterface> &list, CamInterface &templist)
{
    bool id_exsist_flag = false;
    vector<CamInterface>::iterator interface_it;
    for(interface_it = list.begin() ; interface_it != list.end() ; interface_it++)
    {
        if(interface_it->interface_name == templist.interface_name)
            id_exsist_flag = true;
    }

    if(id_exsist_flag)
    {
        for(interface_it = list.begin() ; interface_it != list.end() ; interface_it++)
        {
            if(interface_it->interface_name == templist.interface_name)  // Find same name in list --> manage
            {
                vector<CamInfo>::iterator cam_it;
                //Erase unknown Camera
                for(cam_it = interface_it->camera_list.begin() ; cam_it != interface_it->camera_list.end() ;) //Erase missing camera on list
                {
                    vector<CamInfo>::iterator tempcam_it;
                    bool cam_exsist_flag = false;
                    for(tempcam_it = templist.camera_list.begin() ; tempcam_it != templist.camera_list.end() ; tempcam_it++)
                    {
                        if(tempcam_it->SerialNumber == cam_it->SerialNumber)
                        {
                            cam_exsist_flag = true;
                            break;
                        }
                    }
                    if(!cam_exsist_flag)
                    {
//                        Q_EMIT sendMessage("Erase " + cam_it->ModelName + "/ " + cam_it->SerialNumber , Qt::white);
                        cam_it = interface_it->camera_list.erase(cam_it);
                    }
                    else
                        cam_it++;
                }

                //Push New Camera
                vector<CamInfo>::iterator tempcam_it;
                for(tempcam_it = templist.camera_list.begin() ; tempcam_it != templist.camera_list.end() ; tempcam_it++)
                {
                    bool cam_exsist_flag = false;
                    for(cam_it = interface_it->camera_list.begin() ; cam_it != interface_it->camera_list.end() ; cam_it++)
                    {
                        if(tempcam_it->SerialNumber == cam_it->SerialNumber)
                        {
                            cam_exsist_flag = true;
                            break;
                        }
                    }
                    if(!cam_exsist_flag)
                    {
//                        Q_EMIT sendMessage("Push " + tempcam_it->ModelName + "/ " + tempcam_it->SerialNumber , Qt::white);
                        interface_it->camera_list.push_back(*tempcam_it);
                    }
                    else
                    {
                        *cam_it = *tempcam_it;
                    }
                }
                break;
            }
        }
    }
    else // Not in list --> push back
    {
        list.push_back(templist);
    }
}

gcstring CamManager::GetDottedAddress(int64_t value)
{
    // Helper function for formatting IP Address into the following format
    // x.x.x.x
    unsigned int inputValue = static_cast<unsigned int>(value);
    ostringstream convertValue;
    convertValue << ((inputValue & 0xFF000000) >> 24);
    convertValue << ".";
    convertValue << ((inputValue & 0x00FF0000) >> 16);
    convertValue << ".";
    convertValue << ((inputValue & 0x0000FF00) >> 8);
    convertValue << ".";
    convertValue << (inputValue & 0x000000FF);
    return convertValue.str().c_str();
}

gcstring CamManager::GetMACAddress(int64_t value)
{
    // Helper function for formatting MAC Address into the following format
    // XX:XX:XX:XX:XX:XX
    long long inputValue = static_cast<long long>(value);
    ostringstream convertValue;
    convertValue << std::uppercase << std::setfill('0') << std::setw(2) << std::hex
                 << ((inputValue & 0xFF0000000000) >> 40);
    convertValue << ":";
    convertValue << std::uppercase << std::setfill('0') << std::setw(2) << std::hex
                 << ((inputValue & 0x00FF00000000) >> 32);
    convertValue << ":";
    convertValue << std::uppercase << std::setfill('0') << std::setw(2) << std::hex
                 << ((inputValue & 0x0000FF000000) >> 24);
    convertValue << ":";
    convertValue << std::uppercase << std::setfill('0') << std::setw(2) << std::hex
                 << ((inputValue & 0x000000FF0000) >> 16);
    convertValue << ":";
    convertValue << std::uppercase << std::setfill('0') << std::setw(2) << std::hex
                 << ((inputValue & 0x00000000FF00) >> 8);
    convertValue << ":";
    convertValue << std::uppercase << std::setfill('0') << std::setw(2) << std::hex << (inputValue & 0x0000000000FF);
    return convertValue.str().c_str();
}

void CamManager::AutoConfigure()
{
    SystemPtr pSystem = System::GetInstance();

    InterfaceList interfaceList = pSystem->GetInterfaces();

    // Iterate through available interfaces and attempt to execute force IP
    for (unsigned int i = 0; i < interfaceList.GetSize(); i++)
    {
        InterfacePtr pInterface = interfaceList.GetByIndex(i);

        INodeMap& nodeMapInterface = pInterface->GetTLNodeMap();

        CEnumerationPtr ptrInterfaceType = nodeMapInterface.GetNode("InterfaceType");
        if (!IsAvailable(ptrInterfaceType) || !IsReadable(ptrInterfaceType))
        {
            Q_EMIT sendMessage("Unable to read InterfaceType for interface at index " + QString::number(i), Qt::red);
            continue;
        }

        if (ptrInterfaceType->GetIntValue() != InterfaceType_GigEVision)
        {
            // Only force IP on GEV interface
            continue;
        }

        CStringPtr ptrInterfaceDisplayName = nodeMapInterface.GetNode("InterfaceDisplayName");
        if (IsAvailable(ptrInterfaceDisplayName) && IsReadable(ptrInterfaceDisplayName))
        {
            gcstring interfaceDisplayName = ptrInterfaceDisplayName->GetValue();
        }
        else
        {
            Q_EMIT sendMessage("Unknown Interface (Display name not readable)", Qt::red);
        }

        CCommandPtr ptrAutoForceIP = nodeMapInterface.GetNode("GevDeviceAutoForceIP");
        if (IsAvailable(ptrAutoForceIP) && IsWritable(ptrAutoForceIP))
        {
            if (!IsWritable(pInterface->TLInterface.DeviceSelector.GetAccessMode()))
            {
                Q_EMIT sendMessage("Unable to write to the DeviceSelector node while forcing IP", Qt::red);
            }
            else
            {
                const int cameraCount = pInterface->GetCameras().GetSize();
                for (int i = 0; i < cameraCount; i++)
                {
                    pInterface->TLInterface.DeviceSelector.SetValue(i);
                    pInterface->TLInterface.GevDeviceAutoForceIP.Execute();
                }
            }
        }
        else
        {
            Q_EMIT sendMessage("Warning : Force IP node not available for this interface", Qt::yellow);
        }
    }

    interfaceList.Clear();
    pSystem->ReleaseInstance();
}

void CamManager::QueryInterface(InterfacePtr pInterface)
{
    INodeMap& nodeMapInterface = pInterface->GetTLNodeMap();
    CamInterface temp_list;

    CEnumerationPtr ptrInterfaceType = nodeMapInterface.GetNode("InterfaceType");
    if (IsAvailable(ptrInterfaceType) && IsReadable(ptrInterfaceType))
    {
        if (ptrInterfaceType->GetIntValue() != InterfaceType_GigEVision)
        {
            // Only display GEV interface and devices
            return;
        }
    }

    CStringPtr ptrInterfaceDisplayName = nodeMapInterface.GetNode("InterfaceDisplayName");

    if (IsAvailable(ptrInterfaceDisplayName) && IsReadable(ptrInterfaceDisplayName))
    {
        gcstring interfaceDisplayName = ptrInterfaceDisplayName->GetValue();
        temp_list.interface_name = QString(interfaceDisplayName.c_str());
    }
    else
    {
        Q_EMIT sendMessage("Unknown Interface (Display name not readable)", Qt::red);
    }

    CameraList camList = pInterface->GetCameras();

    // Retrieve number of cameras
    unsigned int numCameras = camList.GetSize();
    // Return if no cameras detected

    if (numCameras == 0)
    {
        vector<CamInterface>::iterator interface_it;
        for(interface_it = interface_list_.begin() ; interface_it != interface_list_.end();)
        {
            if(temp_list.interface_name == interface_it->interface_name)
            {
                interface_it = interface_list_.erase(interface_it);
//                Q_EMIT sendMessage("Erase " + temp_list.interface_name, Qt::white);
                break;
            }
            else
            {
                interface_it++;
            }
        }
        return;
    }
    else
    {
        for (unsigned int i = 0; i < numCameras; i++)
        {
            CameraPtr pCamera = camList.GetByIndex(i);

            temp_list.camera_list.push_back(PrintDeviceInfo(pCamera));
        }
        manageList(interface_list_, temp_list);
    }
    camList.Clear();
}

CamInfo CamManager::PrintDeviceInfo(CameraPtr pCamera)
{
    try
    {
        CamInfo temp_camera;
        INodeMap& nodeMapTLDevice = pCamera->GetTLDeviceNodeMap();

        CStringPtr ptrStringSerial = nodeMapTLDevice.GetNode("DeviceSerialNumber");
        if (IsAvailable(ptrStringSerial) && IsReadable(ptrStringSerial))
        {
            gcstring deviceSerialNumber = ptrStringSerial->GetValue();
            temp_camera.SerialNumber = QString(deviceSerialNumber.c_str());
        }

        CStringPtr ptrDeviceModelName = nodeMapTLDevice.GetNode("DeviceModelName");
        if (IsAvailable(ptrDeviceModelName) && IsReadable(ptrDeviceModelName))
        {
            gcstring deviceModelName = ptrDeviceModelName->ToString();
            temp_camera.ModelName = QString(deviceModelName.c_str());
        }

        CStringPtr ptrDeviceVendorName = nodeMapTLDevice.GetNode("DeviceVendorName");
        if (IsAvailable(ptrDeviceVendorName) && IsReadable(ptrDeviceVendorName))
        {
            gcstring deviceVendorName = ptrDeviceVendorName->ToString();
            temp_camera.VendorName = QString(deviceVendorName.c_str());
        }

        CIntegerPtr ptrIPAddress = nodeMapTLDevice.GetNode("GevDeviceIPAddress");
        if (IsAvailable(ptrIPAddress) && IsReadable(ptrIPAddress))
        {
            gcstring deviceIPAddress = GetDottedAddress(ptrIPAddress->GetValue());
            temp_camera.IPAddress = QString(deviceIPAddress.c_str());
        }

        CIntegerPtr ptrSubnetMask = nodeMapTLDevice.GetNode("GevDeviceSubnetMask");
        if (IsAvailable(ptrSubnetMask) && IsReadable(ptrSubnetMask))
        {
            gcstring deviceSubnetMask = GetDottedAddress(ptrSubnetMask->GetValue());
            temp_camera.SubnetMask = QString(deviceSubnetMask.c_str());
        }

        CIntegerPtr ptrGateway = nodeMapTLDevice.GetNode("GevDeviceGateway");
        if (IsAvailable(ptrGateway) && IsReadable(ptrGateway))
        {
            gcstring deviceGateway = GetDottedAddress(ptrGateway->GetValue());
            temp_camera.Gateway = QString(deviceGateway.c_str());
        }

        CIntegerPtr ptrMACAddress = nodeMapTLDevice.GetNode("GevDeviceMACAddress");
        if (IsAvailable(ptrMACAddress) && IsReadable(ptrMACAddress))
        {
            gcstring deviceMACAddress = GetMACAddress(ptrMACAddress->GetValue());
            temp_camera.MACAddress = QString(deviceMACAddress.c_str());
        }

        try
        {
            // Initialize Camera
            pCamera->Init();
            // Retrieve device nodemap
            INodeMap& nodeMapDevice = pCamera->GetNodeMap();

            CIntegerPtr ptrPersistentIPAddress = nodeMapDevice.GetNode("GevPersistentIPAddress");
            if (IsAvailable(ptrPersistentIPAddress) && IsReadable(ptrPersistentIPAddress))
            {
                gcstring devicePersistentIPAddress = GetDottedAddress(ptrPersistentIPAddress->GetValue());
            }

            CIntegerPtr ptrPersistentSubnetMask = nodeMapDevice.GetNode("GevPersistentSubnetMask");
            if (IsAvailable(ptrPersistentSubnetMask) && IsReadable(ptrPersistentSubnetMask))
            {
                gcstring devicePersistentSubnetMask = GetDottedAddress(ptrPersistentSubnetMask->GetValue());
            }

            CIntegerPtr ptrPersistentGateway = nodeMapDevice.GetNode("GevPersistentDefaultGateway");
            if (IsAvailable(ptrPersistentGateway) && IsReadable(ptrPersistentGateway))
            {
                gcstring devicePersistentGateway = GetDottedAddress(ptrPersistentGateway->GetValue());
            }

            // Deinitialize camera
            pCamera->DeInit();
        }

        catch (Spinnaker::Exception& se)
        {
            if (se == SPINNAKER_ERR_INVALID_ADDRESS)
            {
                Q_EMIT sendMessage("Warning: Camera is on a wrong subnet. Run auto force IP (with -a option) to configure the camera correctly.", Qt::yellow);
            }
            else
            {
                Q_EMIT sendMessage("Error: " + QString(se.what()) , Qt::red);
            }
        }

        return temp_camera;
    }
    catch (Spinnaker::Exception& e)
    {
        Q_EMIT sendMessage("Error: " + QString(e.what()) , Qt::red);
    }
}

vector<string> CamManager::split(const string& str)
{
    // Helper function for splitting string with octets (e.g. x.x.x.x) into
    // a vector of octets with '.' as the delimiter
    string delim = ".";
    vector<string> tokens;
    size_t prev = 0, pos = 0;
    do
    {
        pos = str.find(delim, prev);
        if (pos == string::npos)
        {
            pos = str.length();
        }

        string token = str.substr(prev, pos - prev);
        if (!token.empty())
        {
            tokens.push_back(token);
        }
        prev = pos + delim.length();
    } while (pos < str.length() && prev < str.length());

    return tokens;
}

bool CamManager::IsDigits(const std::string& str)
{
    return str.find_first_not_of("0123456789") == std::string::npos;
}

bool CamManager::ValidateIPV4Address(string ipAddress, bool isSubnetMask = false)
{
    vector<string> ipBytes;
    uint64_t num = 0;

    if (ipAddress.empty() || ipAddress.compare(zeroIPAddress) == 0 ||
            (!isSubnetMask && ipAddress.compare(broadcastAddress) == 0))
    {
        return false;
    }

    // Ensure that the IP address has 4 blocks
    ipBytes = split(ipAddress);
    if (ipBytes.size() != 4)
    {
        return false;
    }

    // Ensure that each block is valid
    for (int i = ipBytes.size() - 1; i >= 0; i--)
    {
        if (ipBytes[i].empty() || !IsDigits(ipBytes[i]))
        {
            return false;
        }

        num = std::atoi(ipBytes[i].c_str());
        if (num < 0 || num > 255)
        {
            return false;
        }
    }

    return true;
}

string CamManager::IPV4ToLongString(string ipAddress)
{
    // Helper function for converting IPV4 to long address representation. The function
    // expects the input IP string to be in the form of 4 octects (e.g. x.x.x.x).
    vector<string> ipBytes;
    uint64_t num = 0;
    if (!ipAddress.empty())
    {
        ipBytes = split(ipAddress);
        for (int i = ipBytes.size() - 1; i >= 0; i--)
        {
            num += ((std::atoi(ipBytes[i].c_str()) % 256) * pow(256, (3 - i)));
        }
    }

    return std::to_string(num);
}

void CamManager::configCamera(QString serial, QString ipAddress, QString netMask, QString gateway)
{
    config_flag = true;
    config_data_[0] = serial.toStdString();
    config_data_[1] = ipAddress.toStdString();
    config_data_[2] = netMask.toStdString();
    config_data_[3] = gateway.toStdString();
}

void CamManager::ConfigureCamera(std::string serial, std::string ipAddress, std::string netMask, std::string gateway)
{
    // Configure the camera if all options are valid
    if (serial.empty() || serial.compare(zeroSerial) == 0)
    {
        Q_EMIT sendMessage("Error: Please specify a valid serial number", Qt::red);
        return;
    }

    if (!(ValidateIPV4Address(ipAddress) && ValidateIPV4Address(netMask, true) && ValidateIPV4Address(gateway)))
    {
        Q_EMIT sendMessage("Error: Invalid IP address, subnet mask or default gateway", Qt::red);
        return;
    }

    // Retrieve singleton reference to system object
    SystemPtr system = System::GetInstance();

    // Retrieve list of cameras from the system
    CameraList camList = system->GetCameras();

    CameraPtr pCam = nullptr;
    pCam = camList.GetBySerial(serial);

    if (pCam == nullptr)
    {
        Q_EMIT sendMessage("Error: Could not find a camera with serial number: " + QString::fromStdString(serial) , Qt::red);
    }
    else
    {
        try
        {
            // Initialize camera
            pCam->Init();

            // Retrieve GenICam nodemap
            INodeMap& nodeMap = pCam->GetNodeMap();

            // Retrieve TL device nodemap
            INodeMap& nodeMapTLDevice = pCam->GetTLDeviceNodeMap();

            CEnumerationPtr ptrDeviceType = nodeMapTLDevice.GetNode("DeviceType");

            if (ptrDeviceType->GetIntValue() != DeviceType_GigEVision)
            {
                Q_EMIT sendMessage("Warning: Persistent IP can only be set for GigE vision cameras" , Qt::yellow);
            }
            else
            {
                // Find the current IP address
                CIntegerPtr gevCurrentIPAddress = nodeMap.GetNode("GevCurrentIPAddress");
                if (!IsReadable(gevCurrentIPAddress))
                {
                    Q_EMIT sendMessage("Error: Cannot read the current IP address" , Qt::red);
                }
                else
                {
                    Q_EMIT sendMessage("Current IP Address is " + QString(GetDottedAddress(gevCurrentIPAddress->GetValue(gevCurrentIPAddress)).c_str()), Qt::white);
                }

                // Enable persistent IP
                CBooleanPtr gevCurrentIPConfigurationPersistentIP =
                        nodeMap.GetNode("GevCurrentIPConfigurationPersistentIP");
                if (!IsWritable(gevCurrentIPConfigurationPersistentIP))
                {
                    Q_EMIT sendMessage("Error: Cannot enable persistent IP address", Qt::red);
                }
                else
                {
                    gevCurrentIPConfigurationPersistentIP->SetValue(true);
                    Q_EMIT sendMessage("Persistent IP enabled set to " + QString::number(gevCurrentIPConfigurationPersistentIP->GetValue()), Qt::red);
                }

                // Set and force a persistent IP address
                CIntegerPtr gevPersistentIPAddress = nodeMap.GetNode("GevPersistentIPAddress");
                CIntegerPtr gevDeviceForceIPAddress = nodeMapTLDevice.GetNode("GevDeviceForceIPAddress");
                if (!IsWritable(gevPersistentIPAddress) || !IsWritable(gevDeviceForceIPAddress))
                {
                    Q_EMIT sendMessage("Error: Cannot force a persistent IP address value", Qt::red);
                }
                else
                {
                    uint64_t ipValue = atol(IPV4ToLongString(ipAddress).c_str());
                    gevPersistentIPAddress->SetValue(ipValue);
                    gevDeviceForceIPAddress->SetValue(ipValue);
                    Q_EMIT sendMessage("Persistent IP address set to " + QString(GetDottedAddress(gevPersistentIPAddress->GetValue()).c_str()), Qt::white);
                }

                // Set and force a persistent subnet mask
                CIntegerPtr gevPersistentSubnetMask = nodeMap.GetNode("GevPersistentSubnetMask");
                CIntegerPtr gevDeviceForceSubnetMask = nodeMapTLDevice.GetNode("GevDeviceForceSubnetMask");
                if (!IsWritable(gevPersistentSubnetMask) || !IsWritable(gevDeviceForceSubnetMask))
                {
                    Q_EMIT sendMessage("Error: Cannot force a persistent subnet mask value", Qt::red);
                }
                else
                {
                    uint64_t netMaskValue = atol(IPV4ToLongString(netMask).c_str());
                    gevPersistentSubnetMask->SetValue(netMaskValue);
                    gevDeviceForceSubnetMask->SetValue(netMaskValue);
                    Q_EMIT sendMessage("Persistent subnet mask set to " + QString(GetDottedAddress(gevPersistentSubnetMask->GetValue()).c_str()), Qt::white);
                }

                // Set and force a persistent default gateway
                CIntegerPtr gevPersistentDefaultGateway = nodeMap.GetNode("GevPersistentDefaultGateway");
                CIntegerPtr gevDeviceForceGateway = nodeMapTLDevice.GetNode("GevDeviceForceGateway");
                if (!IsWritable(gevPersistentDefaultGateway) || !IsWritable(gevDeviceForceGateway))
                {
                    Q_EMIT sendMessage("Error: Cannot set and force a persistent default gateway value", Qt::red);
                }
                else
                {
                    uint64_t defaultGatewayValue = atol(IPV4ToLongString(gateway).c_str());
                    gevPersistentDefaultGateway->SetValue(defaultGatewayValue);
                    gevDeviceForceGateway->SetValue(defaultGatewayValue);
                    Q_EMIT sendMessage("Persistent default gateway set to " + QString(GetDottedAddress(gevPersistentDefaultGateway->GetValue()).c_str()), Qt::white);
                }

                // De-initialize the camera
                pCam->DeInit();

                CCommandPtr gevDeviceForceIP = nodeMapTLDevice.GetNode("GevDeviceForceIP");
                if (!IsWritable(gevDeviceForceIP))
                {
                    Q_EMIT sendMessage("Error: Cannot execute device force IP command", Qt::red);
                }
                else
                {
                    gevDeviceForceIP->Execute();
                    Q_EMIT sendMessage("Device force IP command executed", Qt::white);
                }
            }

//            // Ensure that the device is de-initialized
//            if (pCam->IsInitialized())
//            {
//                // De-initialize the camera
//                pCam->DeInit();
//            }
        }
        catch (Spinnaker::Exception& e)
        {
            if (e == SPINNAKER_ERR_INVALID_ADDRESS)
            {
                Q_EMIT sendMessage("Warning: Camera is on a wrong subnet. Run auto force IP (with -a option) to configure the camera correctly.", Qt::yellow);
            }
            else
            {
                Q_EMIT sendMessage("Error: " + QString(e.what()) , Qt::red);
            }
        }
    }
    Q_EMIT sendMessage("IP Configuration Sucess!!!", Qt::green);
    pCam = nullptr;

    // Clear camera list before releasing system
    camList.Clear();

    // Release system
    system->ReleaseInstance();
}

void CamManager::ListAllDeviceInfo()
{
    while(loop_ctrl_)
    {
        SystemPtr system = System::GetInstance();


        InterfaceList interfaceList = system->GetInterfaces();

        unsigned int numInterfaces = interfaceList.GetSize();

        InterfacePtr interfacePtr = nullptr;

        for (unsigned int i = 0; i < numInterfaces; i++)
        {
            // Select interface
            interfacePtr = interfaceList.GetByIndex(i);
            // Query interface
            QueryInterface(interfacePtr);
        }
        interfacePtr = nullptr;

        interfaceList.Clear();

        system->ReleaseInstance();
        Q_EMIT updateList();
//        cout << "<-------- list -------->" << endl;
//        vector<CamInterface>::iterator it1;
//        for(it1 = interface_list_.begin() ; it1 != interface_list_.end(); it1++)
//        {
//            cout << it1->interface_name.toStdString() << ":" << endl;
//            vector<CamInfo>::iterator it2;
//            for(it2 = it1->camera_list.begin() ; it2 != it1->camera_list.end() ; it2++)
//            {
//                cout << it2->SerialNumber.toStdString() << it2->ModelName.toStdString() << endl;
//            }

//        }
//        cout << "---------" << endl;
        if(config_flag)
        {
            AutoConfigure();
            ConfigureCamera(config_data_[0], config_data_[1], config_data_[2], config_data_[3]);
            config_flag = false;
        }
    }
}
