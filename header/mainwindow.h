#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTreeWidget>
#include <QTextEdit>
#include <QScrollBar>
#include "camera_interface.h"
#include "camera_manage.h"
#define DATA_COUNT 8
#define MAX_CAMERA 10

using namespace Spinnaker;
using namespace Spinnaker::GenApi;
using namespace Spinnaker::GenICam;
using namespace std;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void initTree();
    void addCameratotree(uint8_t, CamInfo &);
    void addInterfacetotree(CamInterface &);

    void eraseCameratotree(QString);
    void eraseInterfacetotree(QString, uint8_t);

    CamInfo &getCamdata(QString);

    bool isIteminlist(QString, int);

    uint8_t getAvaliableID();

private slots:
    void configInterfacetree();
    void configCameratree(QTreeWidgetItem *, int);
    void on_pb_ip_clicked();
    void updateMessagebox(QString, QColor);


private:
    Ui::MainWindow *ui;
    CamManager *cam_manager_;
    bool check_id[MAX_CAMERA];
    vector<CamInterface> tree_list_;
    enum Column
    {
        MODEL_NAME, SERIALNUMBER, IP_ADDRESS
    };
};

#endif // MAINWINDOW_H
