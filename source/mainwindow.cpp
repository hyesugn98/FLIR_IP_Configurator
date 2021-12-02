#include "header/mainwindow.h"
#include "ui_mainwindow.h"

using namespace Spinnaker;
using namespace Spinnaker::GenApi;
using namespace Spinnaker::GenICam;
using namespace std;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    cam_manager_ = new CamManager(this);
    setWindowTitle("FLIR IP Configurator");

    for(unsigned int i = 0 ; i < MAX_CAMERA ; i++)
        check_id[i] = false;

    //init UI
    initTree();

    //connect
    connect(cam_manager_ , SIGNAL(sendMessage(QString, QColor)), this, SLOT(updateMessagebox(QString, QColor)));
    connect(cam_manager_ , SIGNAL(updateList()), this, SLOT(configInterfacetree()));
    connect(ui->list_tree, SIGNAL(itemClicked(QTreeWidgetItem *, int)), this, SLOT(configCameratree(QTreeWidgetItem *, int)));
}

MainWindow::~MainWindow()
{
    delete ui;
    delete cam_manager_;
}

void MainWindow::initTree()
{
    QTreeWidgetItem *header_interface = new QTreeWidgetItem;
    header_interface->setText(0, "Interface");
    header_interface->setText(1, "Serial");
    header_interface->setText(2, "IP Address");
    ui->list_tree->setHeaderItem(header_interface);

    QTreeWidgetItem *header_camera = new QTreeWidgetItem;
    header_camera->setText(0, "Model Name");
    header_camera->setText(1, "Data");
}

void MainWindow::configInterfacetree()
{
    vector<CamInterface> interface_list = cam_manager_->getInterfacelist();
    vector<CamInterface>::iterator interface_it;

    /** Erase Item **/
    vector<CamInterface>::iterator tree_it;
    for(tree_it = tree_list_.begin() ; tree_it != tree_list_.end() ;)
    {
        bool interface_exsist = false;
        for(interface_it = interface_list.begin() ; interface_it != interface_list.end() ; interface_it++)
        {
            if(tree_it->interface_name == interface_it->interface_name)
            {
                interface_exsist = true;
                break;
            }
        }
        if(!interface_exsist) //erase interface
        {
            if(isIteminlist(tree_it->interface_name, 0))
            {
//                    cout << "ERASE " << tree_it->interface_name.toStdString() << " " << (int)tree_it->interface_id << endl;
                    eraseInterfacetotree(tree_it->interface_name, tree_it->interface_id);
                    check_id[tree_it->interface_id] = false;
                    tree_it = tree_list_.erase(tree_it);
            }
        }
        else //erase camera
        {
            vector<CamInfo>::iterator treecam_it;
            for(treecam_it = tree_it->camera_list.begin() ; treecam_it != tree_it->camera_list.end() ;)
            {
                vector<CamInfo>::iterator cam_it;
                bool cam_exsist = false;
                for(cam_it = interface_it->camera_list.begin() ; cam_it != interface_it->camera_list.end() ; cam_it++)
                {
                    if(treecam_it->SerialNumber == cam_it->SerialNumber)
                    {
                        cam_exsist = true;
                        break;
                    }
                }
                if(!cam_exsist)
                {
                    if(isIteminlist(treecam_it->SerialNumber, 1))
                    {
                        eraseCameratotree(treecam_it->SerialNumber);
                        treecam_it = tree_it->camera_list.erase(treecam_it);
                    }
                }
                else
                    treecam_it++;
            }
            tree_it++;
        }
    }

    /** Add Item **/
    for(interface_it = interface_list.begin() ; interface_it != interface_list.end() ; interface_it++)
    {
        bool interface_exsist = false;
        vector<CamInterface>::iterator tree_it;
        for(tree_it = tree_list_.begin() ; tree_it != tree_list_.end() ; tree_it++)
        {
            if(tree_it->interface_name == interface_it->interface_name)
            {
                interface_exsist = true;
                break;
            }
        }

        if(interface_exsist)
        {
            vector<CamInfo>::iterator cam_it;
            bool cam_exsist = false;
            for(cam_it = interface_it->camera_list.begin() ; cam_it != interface_it->camera_list.end() ; cam_it++)
            {
                vector<CamInfo>::iterator treecam_it;
                for(treecam_it = tree_it->camera_list.begin() ; treecam_it  != tree_it->camera_list.end() ; treecam_it++)
                {
                    if(treecam_it->SerialNumber == cam_it->SerialNumber)
                    {
                        cam_exsist = true;
                        break;
                    }
                }
                if(!cam_exsist)
                {
                    tree_it->camera_list.push_back(*cam_it);
                    addCameratotree(tree_it->interface_id, *cam_it);
                }
                else
                {
                    eraseCameratotree(treecam_it->SerialNumber);
                    addCameratotree(tree_it->interface_id, *cam_it);
                    *treecam_it = *cam_it;
                }
            }
        }
        else
        {
            if(isIteminlist(interface_it->interface_name, 0))
            {
                eraseInterfacetotree(interface_it->interface_name, interface_it->interface_id);
            }
            addInterfacetotree(*interface_it);
            vector<CamInfo>::iterator intercam_it;
            for(intercam_it = interface_it->camera_list.begin() ; intercam_it  != interface_it->camera_list.end() ; intercam_it++)
            {
                addCameratotree(interface_it->interface_id, *intercam_it);
            }
//            cout << "ADD " << interface_it->interface_name.toStdString() << (int)interface_it->interface_id << endl;
            tree_list_.push_back(*interface_it);
        }
    }
//    cout << "<-------- UI -------->" << endl;
//    vector<CamInterface>::iterator it1;
//    for(it1 = tree_list_.begin() ; it1 != tree_list_.end(); it1++)
//    {
//        cout << it1->interface_name.toStdString() << ":" << endl;
//        vector<CamInfo>::iterator it2;
//        for(it2 = it1->camera_list.begin() ; it2 != it1->camera_list.end() ; it2++)
//        {
//            cout << it2->SerialNumber.toStdString() << it2->ModelName.toStdString() << endl;
//        }

//    }
//    cout << "---------" << endl;
}

CamInfo &MainWindow::getCamdata(QString serial_num)
{
    vector<CamInterface>::iterator tree_it;
    for(tree_it = tree_list_.begin() ; tree_it != tree_list_.end() ; tree_it ++)
    {
        vector<CamInfo>::iterator cam_it;
        for(cam_it = tree_it->camera_list.begin() ; cam_it != tree_it->camera_list.end() ; cam_it++)
        {
            if(cam_it->SerialNumber == serial_num)
            {
                return *cam_it;
            }
        }
    }
}

void MainWindow::configCameratree(QTreeWidgetItem *item, int column)
{
    if(!item->text(Column::MODEL_NAME).contains("GEV", Qt::CaseInsensitive))
    {
        CamInfo curcam_info = getCamdata(item->text(Column::SERIALNUMBER));
        if(!curcam_info.ModelName.isNull())
        {
            ui->te_ip->setText(curcam_info.IPAddress);
            ui->te_serial->setText(curcam_info.SerialNumber);
            ui->te_mac->setText(curcam_info.MACAddress);
            ui->te_gate->setText(curcam_info.Gateway);
            ui->te_mask->setText(curcam_info.SubnetMask);
            ui->te_vendor->setText(curcam_info.VendorName);
        }
    }
    else
    {
        ui->te_ip->clear();
        ui->te_serial->clear();
        ui->te_mac->clear();
        ui->te_gate->clear();
        ui->te_mask->clear();
        ui->te_vendor->clear();
    }
}

bool MainWindow::isIteminlist(QString text, int column)
{
    QList<QTreeWidgetItem*> item = ui->list_tree->findItems(text, Qt::MatchContains|Qt::MatchRecursive, column);
    if(item.size() > 0)
        return true;
    else
        return false;
}

void MainWindow::eraseInterfacetotree(QString text, uint8_t id)
{
    vector<CamInterface>::iterator tree_it;
    for(tree_it = tree_list_.begin() ; tree_it != tree_list_.end() ; tree_it ++)
    {
        if(text != tree_it->interface_name)
        {
            if(tree_it->interface_id > id)
            {
                check_id[tree_it->interface_id] = false;
                tree_it->interface_id--;
                check_id[tree_it->interface_id] = true;
            }
        }
    }
    QList<QTreeWidgetItem*> item = ui->list_tree->findItems(text, Qt::MatchContains|Qt::MatchRecursive, 0);
    for(int i = 0; i< item.count(); i++)
    {
        delete item[i];
    }
}

void MainWindow::eraseCameratotree(QString text)
{
    QList<QTreeWidgetItem*> item = ui->list_tree->findItems(text, Qt::MatchContains|Qt::MatchRecursive, 1);
    for(int i = 0; i< item.count(); i++)
    {
        delete item[i];
    }
}

void MainWindow::addCameratotree(uint8_t interface_id, CamInfo &cam_info)
{
    QTreeWidgetItem *child = new QTreeWidgetItem;
    child->setText(0, cam_info.ModelName);
    child->setText(1, cam_info.SerialNumber);
    child->setText(2, cam_info.IPAddress);
//    cout << "ADD in " << int(interface_id) << endl;
    ui->list_tree->topLevelItem(int(interface_id))->addChild(child);
}

void MainWindow::addInterfacetotree(CamInterface &temp_interface)
{
    QTreeWidgetItem *col = new QTreeWidgetItem(ui->list_tree);
    uint8_t id =  getAvaliableID();
    temp_interface.interface_id = id;
    col->setText(0, temp_interface.interface_name);
    ui->list_tree->addTopLevelItem(col);
}

void MainWindow::on_pb_ip_clicked()
{
    QString ipAddress = ui->te_ip->toPlainText();
    QString serialNumber = ui->te_serial->toPlainText();
    QString subnetMask = ui->te_mask->toPlainText();
    QString gateWay = ui->te_gate->toPlainText();
    cam_manager_->configCamera(serialNumber, ipAddress, subnetMask, gateWay);
}

void MainWindow::updateMessagebox(QString msg, QColor color)
{
    QTextCharFormat fmt;
    fmt.setForeground(QBrush(color));
    ui->messagebox->mergeCurrentCharFormat(fmt);
    ui->messagebox->appendPlainText(msg);
    QScrollBar *sb = ui->messagebox->verticalScrollBar();
    sb->setValue(sb->maximum());
}

uint8_t MainWindow::getAvaliableID()
{
    for(uint8_t i = 0 ; i < MAX_CAMERA ; i++)
    {
        if(!check_id[i]){
            check_id[i] = true;
            return i;
        }
    }
    return MAX_CAMERA;
}

