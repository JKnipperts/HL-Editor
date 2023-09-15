/*
 * HL Editor v. 0.1
 * by Jan Knipperts (Dragonsphere /DOSReloaded)
 *
 * A map editor for History Line 1914-1918 by BlueByte
 * Still under development. Many functions are still missing or not finished yet.
 */



#include "mainwindow.h"
#include <QApplication>
#include <QtWidgets>
#include <QMessageBox>
#include <QInputDialog>
#include <QScrollArea>
#include <QMouseEvent>


//Some global variables, string constants etc.

QString                  GameDir;                           //Path to History Line 1914-1918 (read from config file)
QString                  Map_file;                          //String for user selected map file
QString                  MapDir       = "/MAP";             //Maps should be in the MAP sub directory of the game
QString                  Palette_name = "/00.PAL";          //Standard VGA Palette file of the game
QString                  Partlib_name = "/LIB/PARTS.LIB";   //Game ressource files
QString                  Partdat_name = "/LIB/PARTS.DAT";
QString                  Unitlib_name = "/LIB/UNIT.LIB";
QString                  Unitdat_name = "/LIB/UNIT.DAT";
QString                  Cfg = "/CONFIG.CFG";               //Our config file


QImage                   MapImage;                          //QImage as Buffer to draw the map
QImage                   MapImageScaled;                    //Buffer for scaled map image
int                      Scale_factor = 2;                  //Default scaling factor for VGA bitmaps
bool                     Res_loaded = false;                //to check if bitmaps have already been loaded into memory


//now include the code to load game ressources and so on

#include "Lib.h"
#include "fin.h"
#include "other.h"




//=================== Main Window stuff ==========================

//Creates Main Window and adds a scroll area to display maps
MainWindow::MainWindow()
{
    createActions();
    createMenus();
    setWindowTitle(tr("HL 1914-1918 Editor"));
    setMinimumSize(160, 160);
    resize(800, 600);

    //initialize images and the scroll area
    MapImage = QImage(); //Create a new QImage object for the map image
    MapImageScaled =QImage(); //Create a scaled version of it
    QLabel *imageLabel = new QLabel;     //Create a scroll area to display the map
    imageLabel->setPixmap(QPixmap::fromImage(MapImageScaled));
    scrollArea = new(QScrollArea);
    scrollArea->setBackgroundRole(QPalette::Dark);
    scrollArea->setWidget(imageLabel);
    scrollArea->setVisible(true);
    setCentralWidget(scrollArea);
}

//Own closeEvent handler to make sure allocated memory will be properly released
void MainWindow::closeEvent(QCloseEvent *event)
{
    Release_Buffers();
}


void MainWindow::mousePressEvent(QMouseEvent *event)
{
    //Check if the button mouse that was clicked was the left one
    if (event->button() == Qt::LeftButton)
    {
      Map.loaded = true;
        if (Map.loaded == true)
        {
            QPainter painter(&MapImageScaled);
            painter.setPen(QPen(Qt::red, Scale_factor));
            //painter.drawPoint(event->pos().x()+scrollArea->horizontalScrollBar()->value(),event->pos().y()+scrollArea->verticalScrollBar()->value());
            painter.drawPoint(event->pos().x(),event->pos().y()-10);
            painter.end();

            QLabel *imageLabel = new QLabel;     //Create a scroll area to display the map
            imageLabel->setPixmap(QPixmap::fromImage(MapImageScaled));
            if (scrollArea == NULL) scrollArea = new(QScrollArea);
            scrollArea->setWidget(imageLabel);

        }
    }
}



//========== Context menu ======================

#ifndef QT_NO_CONTEXTMENU
void MainWindow::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu(this);
    menu.exec(event->globalPos());
}
#endif // QT_NO_CONTEXTMENU



void MainWindow::newFile()
{

    if (Res_loaded == false)
    {
        if (Load_Ressources() != 0) return;
    }



    if (Map.data != NULL) free(Map.data);
    Map.width = 21;
    Map.height = 25;
    Map.data_size = ((Map.width+1) * (Map.height+1)) * 2;


    if ((Map.data = (unsigned char*)malloc(Map.data_size)) == NULL)
    {
        QMessageBox              Errormsg;
        Errormsg.critical(0,"Error","Memory allocation error!");
        Errormsg.setFixedSize(500,200);
        return;
    }
    else
    {

        memset(Map.data, 0x00FF, Map.data_size);  //Clear all terrain and unit data


        int x,y;
        for (y = 1; y <= Map.height; y++)
        {
            Map.data[(y*Map.width*2)] = (unsigned short int) 0x00AE;
        }
        for (x = 0; x < Map.width; x++)
        {
            Map.data[(Map.height*(Map.width*2)+(x*2))] = (unsigned short int) 0x00AE;
        }

        if (!MapImage.isNull()) MapImage = QImage(); //Release mem for the last used image
        if (!MapImageScaled.isNull()) MapImageScaled = QImage(); //Release mem for the last used scaled image
        MapImage = QImage((Map.width*Tilesize)-((Map.width-1)*Tileshift),(Map.height*Tilesize)+(Tilesize/2), QImage::Format_RGB16); //Create a new QImage object for the map image
        MapImage.fill(Qt::transparent);
        MapImageScaled = MapImage.scaled(MapImage.width()*Scale_factor,MapImage.height()*Scale_factor); //Create a scaled version of it
        Map.loaded = true; //Blank map loaded successfully ;)
        ShowGrid();  //Draw a Hexfield-Grid on it

        QLabel *imageLabel = new QLabel;     //Update the scrollArea
        imageLabel->setPixmap(QPixmap::fromImage(MapImageScaled));
        if (scrollArea == NULL) scrollArea = new(QScrollArea);
        scrollArea->setWidget(imageLabel);

        showgridAct->setChecked(true);

    }
}

void MainWindow::open()
{
    Map_file = QFileDialog::getOpenFileName(this,
                                            tr("Open History Line 1914-1918 map file"),MapDir, tr("HL map files (*.fin)"));
    if (Map_file != "")
    {
        if (Res_loaded == false)
        {
            if (Load_Ressources() != 0) return;
        }

        if (Load_Map() == 0)
        {
            MapImage = QImage(((Map.width/2)*Tilesize)+(((Map.width/2)-1)*Tileshift),((Map.height-1)*Tilesize)+(Tilesize/2), QImage::Format_RGB16); //Create a new QImage object for the map image
            MapImage.fill(Qt::transparent);
            Draw_Map(); //and draw the map to it
            MapImageScaled = MapImage.scaled(MapImage.width()*Scale_factor,MapImage.height()*Scale_factor); //Create a scaled version of it

            QLabel *imageLabel = new QLabel;     //Create a scroll area to display the map
            imageLabel->setPixmap(QPixmap::fromImage(MapImageScaled));

            scrollArea->setWidget(imageLabel);

            Map.loaded = true;
        }
    }

}

void MainWindow::save()
{
    if (Map.loaded == true)
    {
        Map_file = QFileDialog::getSaveFileName(this,
                   tr("Save History Line 1914-1918 map file"),MapDir, tr("HL map files (*.fin)"));
        if (Map_file != "")
        {
            Save_Mapdata(Map_file.toStdString().data());
        }
    }
}


void MainWindow::grid()
{
    if(showgridAct->isChecked())
    {
        ShowGrid();  //Draw a Hexfield-Grid
    }
    else
    {
        MapImageScaled = MapImage.scaled(MapImage.width()*Scale_factor,MapImage.height()*Scale_factor);
    }

    QLabel *imageLabel = new QLabel;     //Update the scrollArea
    imageLabel->setPixmap(QPixmap::fromImage(MapImageScaled));
    if (scrollArea == NULL) scrollArea = new(QScrollArea);
    scrollArea->setWidget(imageLabel);
}



void MainWindow::setPath()
{
    QMessageBox    Errormsg;
    QDir           dir;


    GameDir = QFileDialog::getExistingDirectory(this, tr("Please select directory of the game Historyline 1914-1918"),
                                                GameDir,
                                                QFileDialog::ShowDirsOnly
                                                    | QFileDialog::DontResolveSymlinks);

    if (!Check_for_game_files())
    {
        Errormsg.warning(0,"","I cannot find the required game files in the selected directory!");
        Errormsg.setFixedSize(500,200);
    }
    else
    {

        QFile cfgFile(dir.currentPath()+Cfg);
        cfgFile.open(QIODevice::WriteOnly);

        if (!cfgFile.isOpen())
        {
            Errormsg.warning(0,"","I cannot save the configuration file!");
            Errormsg.setFixedSize(500,200);
        }
        else
        {
            QTextStream out(&cfgFile);
            out << GameDir + "\n";
            out << Scale_factor;
            cfgFile.close();

            MapDir = GameDir+MapDir;
        }

    }

}

void MainWindow::setScale()
{
     bool ok;
    Scale_factor = QInputDialog::getInt(
        this,
        tr("VGA scaler:"),
        tr("Enter a factor for scaling"),
        Scale_factor,
        0,
        10,
        1,
        &ok );

     if (ok)
    {
        if (Scale_factor < 1) Scale_factor = 1;     
        MapImageScaled = MapImage.scaled(MapImage.width()*Scale_factor,MapImage.height()*Scale_factor);
        QLabel *imageLabel = new QLabel;
        imageLabel->setPixmap(QPixmap::fromImage(MapImageScaled));
        scrollArea->setWidget(imageLabel);
     }

}
void MainWindow::createActions()
{
    newAct = new QAction(tr("&New"), this);
    newAct->setShortcuts(QKeySequence::New);
    newAct->setStatusTip(tr("Create a new map"));
    connect(newAct, &QAction::triggered, this, &MainWindow::newFile);

    openAct = new QAction(tr("&Open..."), this);
    openAct->setShortcuts(QKeySequence::Open);
    openAct->setStatusTip(tr("Open an existing map"));
    connect(openAct, &QAction::triggered, this, &MainWindow::open);

    saveAct = new QAction(tr("&Save"), this);
    saveAct->setShortcuts(QKeySequence::Save);
    saveAct->setStatusTip(tr("Save the map to disk"));
    connect(saveAct, &QAction::triggered, this, &MainWindow::save);

    exitAct = new QAction(tr("E&xit"), this);
    exitAct->setShortcuts(QKeySequence::Quit);
    exitAct->setStatusTip(tr("Exit HL Editor"));
    connect(exitAct, &QAction::triggered, this, &QWidget::close);

    showgridAct = new QAction(tr("Grid"), this);
    showgridAct->setCheckable(true);
    showgridAct->setChecked(false);
    showgridAct->setStatusTip(tr("Show/Hide grid"));
    connect(showgridAct,&QAction::triggered,this,&MainWindow::grid);

    setPathAct = new QAction(tr("&Game path"),this);
    setPathAct->setStatusTip(tr("Set path to game resources"));
    connect(setPathAct,&QAction::triggered,this,&MainWindow::setPath);

    setScaleFactorAct = new QAction(tr("&Scale factor"),this);
    setScaleFactorAct->setStatusTip(tr("Scaler used to enlarge/enhance low resolution bitmaps."));
    connect(setScaleFactorAct,&QAction::triggered,this,&MainWindow::setScale);
}

void MainWindow::createMenus()
{
    fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(newAct);
    fileMenu->addAction(openAct);
    fileMenu->addAction(saveAct);
    fileMenu->addSeparator();
    fileMenu->addAction(exitAct);

    editMenu =  menuBar()->addMenu(tr("&Edit"));
    editMenu->addAction(showgridAct);

    configMenu = menuBar()->addMenu(tr("&Settings"));
    configMenu->addAction(setPathAct);
    configMenu->addAction(setScaleFactorAct);
}




int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    MainWindow window;
    window.show();

    if ((!Read_Config()) || (!Check_for_game_files()))
    {
        QMessageBox              Errormsg;
        Errormsg.warning(0,"","I cannot find the required game files in "+GameDir+"! Please reconfigure directories in the settings.");
        Errormsg.setFixedSize(500,200);
    }
    return app.exec();
}


