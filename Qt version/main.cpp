/*
 * HL Editor v. 0.1
 * by Jan Knipperts (Dragonsphere /DOSReloaded)
 *
 * A map editor for History Line 1914-1918 by BlueByte
 * Still under development. Many functions are still missing or not finished yet.
 */



#include "mainwindow.h"
#include "tilelist.h"
#include "unitlist.h"
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

QImage                   TileListImage;
QImage                   TileListImageScaled;
QScrollArea              *tilescrollArea;
QWidget                  *tile_selection;

QImage                   UnitListImage;
QImage                   UnitListImageScaled;
QScrollArea              *unitscrollArea;
QWidget                  *unit_selection;
QRect                    screenrect;

int                      Scale_factor = 2;                  //Default scaling factor for VGA bitmaps
unsigned char            selected_tile = 0x00;
unsigned char            selected_unit = 0xFF;
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
    setWindowTitle(tr("HL 1914-1918 Editor by Jan Knipperts - Version 0.1 alpha"));

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
        if (Map.loaded == true)
        {
            int pos_x = scrollArea->horizontalScrollBar()->value(); //save scroll position
            int pos_y = scrollArea->verticalScrollBar()->value();
            int fx = (event->pos().x()+pos_x) / ((Tilesize-Tileshift)*Scale_factor); // Calc field position from mouse cords
            int fy = ((event->pos().y()-20)+pos_y) / (Tilesize*Scale_factor); //-20 for menu bar size...
            int field_pos = (fy*Map.width)+fx;

            Map.data[field_pos*2] = (unsigned char) selected_tile;
            Map.data[(field_pos*2)+1] = (unsigned char) selected_unit;
            Redraw_Field(fx,fy,selected_tile,selected_unit);


            MapImageScaled = MapImage.scaled(MapImage.width()*Scale_factor,MapImage.height()*Scale_factor); //Create a scaled version of it
            if(showgridAct->isChecked()) ShowGrid();  //redraw the grid if enabled
            Draw_Hexagon(fx,fy,QPen(Qt::red, 1),&MapImageScaled,true,true); //redraw the frame


            QLabel *imageLabel = new QLabel;     //Create a scroll area to display the map
            imageLabel->setPixmap(QPixmap::fromImage(MapImageScaled));
            scrollArea->setWidget(imageLabel);

            scrollArea->horizontalScrollBar()->setValue(pos_x); //Reset the scrollArea to last position
            scrollArea->verticalScrollBar()->setValue(pos_y);

        }
    }

    if (event->button() == Qt::RightButton)
    {
        if (Map.loaded == true)
        {
            int pos_x = scrollArea->horizontalScrollBar()->value(); //save scroll position
            int pos_y = scrollArea->verticalScrollBar()->value();
            int fx = (event->pos().x()+pos_x) / ((Tilesize-Tileshift)*Scale_factor); // Calc field position from mouse cords
            int fy = ((event->pos().y()-20)+pos_y) / (Tilesize*Scale_factor); //-20 for menu bar size...
            int field_pos = (fy*Map.width)+fx;

            Map.data[field_pos*2] = (unsigned char) selected_tile;
            Map.data[(field_pos*2)+1] = (unsigned char) 0xFF;  //Clear unit
            Redraw_Field(fx,fy,selected_tile,selected_unit);


            MapImageScaled = MapImage.scaled(MapImage.width()*Scale_factor,MapImage.height()*Scale_factor); //Create a scaled version of it
            if(showgridAct->isChecked()) ShowGrid();  //redraw the grid if enabled
            Draw_Hexagon(fx,fy,QPen(Qt::red, 1),&MapImageScaled,true,true); //redraw the frame


            QLabel *imageLabel = new QLabel;     //Create a scroll area to display the map
            imageLabel->setPixmap(QPixmap::fromImage(MapImageScaled));
            scrollArea->setWidget(imageLabel);

            scrollArea->horizontalScrollBar()->setValue(pos_x); //Reset the scrollArea to last position
            scrollArea->verticalScrollBar()->setValue(pos_y);
        }

    }


}


void tilelistwindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        int pos_x = tilescrollArea->horizontalScrollBar()->value();
        int pos_y = tilescrollArea->verticalScrollBar()->value();
        TileListImageScaled = TileListImage.scaled(TileListImage.width()*Scale_factor,TileListImage.height()*Scale_factor); //Restore original image
        int fx = (event->pos().x()+pos_x) / (Tilesize*Scale_factor); // Calc field position from mouse cords
        int fy = (event->pos().y()+pos_y) / (Tilesize*Scale_factor);
        Draw_Hexagon(fx,fy,QPen(Qt::red, 1),&TileListImageScaled,false,true); //Draw the frame
        selected_tile = (fy*10)+fx;

        QLabel *label = new QLabel();
        label->setPixmap(QPixmap::fromImage(TileListImageScaled));

        tilescrollArea->setWidget(label);
        tile_selection->update();
        tilescrollArea->horizontalScrollBar()->setValue(pos_x); //Reset the scrollArea to last position
        tilescrollArea->verticalScrollBar()->setValue(pos_y);
    }
}


void unitlistwindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        int pos_x = unitscrollArea->horizontalScrollBar()->value();
        int pos_y = unitscrollArea->verticalScrollBar()->value();
        UnitListImageScaled = UnitListImage.scaled(UnitListImage.width()*Scale_factor,UnitListImage.height()*Scale_factor); //Restore original image
        int fx = (event->pos().x()+pos_x) / (Tilesize*Scale_factor); // Calc field position from mouse cords
        int fy = (event->pos().y()+pos_y) / (Tilesize*Scale_factor);

        selected_unit = ((fy*10)+fx) * 2;     //Calc correct unit number
        if (selected_unit >= 120)
          selected_unit = selected_unit-119; //correction for french units

        if (selected_unit < (Num_Units*2)) Draw_Hexagon(fx,fy,QPen(Qt::red, 1),&UnitListImageScaled,false,true); //Draw the frame

        QLabel *label = new QLabel();
        label->setPixmap(QPixmap::fromImage(UnitListImageScaled));  //update the image

        unitscrollArea->setWidget(label);
        unit_selection->update();                           //Update the window contents
        unitscrollArea->horizontalScrollBar()->setValue(pos_x); //Reset the scrollArea to last position
        unitscrollArea->verticalScrollBar()->setValue(pos_y);
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

    Map.width = 16; //Set default width and height
    Map.height = 16;
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
        int x,y,o;
        o = 0;
        for (y = 1; y <= Map.height; y++)
        {
          for (x = 1; x <= Map.width; x++)
          {
              if ((x == Map.width) || (y == Map.height))
              {
                  Map.data[o] =  0xAE;
                  Map.data[o+1] =  0xFF;
              }
              else
              {
                  Map.data[o] =  0x00;
                  Map.data[o+1] =  0xFF;
              }

              o = o + 2;
          }
        }


        if (!MapImage.isNull()) MapImage = QImage(); //Release mem for the last used image
        if (!MapImageScaled.isNull()) MapImageScaled = QImage(); //Release mem for the last used scaled image
        MapImage = QImage(((Map.width/2)*Tilesize)+(((Map.width/2)-1)*Tileshift),((Map.height-1)*Tilesize)+(Tilesize/2), QImage::Format_RGB16); //Create a new QImage object for the map image
        MapImage.fill(Qt::transparent);
        Draw_Map(); //and draw the map to it
        MapImageScaled = MapImage.scaled(MapImage.width()*Scale_factor,MapImage.height()*Scale_factor); //Create a scaled version of it
        Map.loaded = true; //Blank map loaded successfully ;)
        ShowGrid();  //Draw a Hexfield-Grid on it
        QLabel *imageLabel = new QLabel;     //Update the scrollArea
        imageLabel->setPixmap(QPixmap::fromImage(MapImageScaled));
        if (scrollArea == NULL) scrollArea = new(QScrollArea);
        scrollArea->setWidget(imageLabel);
        showgridAct->setChecked(true);

        if (showtilewindowAct->isChecked() == true)
        {
          if (tile_selection == NULL)
              Create_Tileselection_window();
          else
              tile_selection->show();
        }

        if (showunitwindowAct->isChecked() == true)
        {
          if (unit_selection == NULL)
              Create_Unitselection_window();
          else
              unit_selection->show();
        }
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

            if (showtilewindowAct->isChecked() == true)
            {
                if (tile_selection == NULL)
                    Create_Tileselection_window();
                else
                    tile_selection->show();
            }

            if (showunitwindowAct->isChecked() == true)
            {
                if (unit_selection == NULL)
                    Create_Unitselection_window();
                else
                    unit_selection->show();
            }
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


void MainWindow::griddiag()
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


void MainWindow::tilewindowdiag()
{
    if(showtilewindowAct->isChecked())
    {
        if (tile_selection == NULL)
            Create_Tileselection_window();
        else
            tile_selection->show();
    }
    else
    {
        if (tile_selection != NULL) tile_selection->close();
    }
}

void MainWindow::unitwindowdiag()
{
    if(showunitwindowAct->isChecked())
    {
        if (unit_selection == NULL)
            Create_Unitselection_window();
        else
            unit_selection->show();
    }
    else
    {
        if (unit_selection != NULL) unit_selection->close();
    }
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


void MainWindow::mapresizediag()
{
    QStringList items;
    items << "16x16"
        << "16x24"
        << "16x32"
        << "16x40"
        << "24x24"
        << "24x32"
        << "32x24"
        << "32x32"
        << "32x48"
        << "40x32"
        << "40x40"
        << "48x64"
        << "64x24"
        << "64x32"
          << "64x48";

    int current;
    current = ((Map.width-8) / 8) + ((Map.height-8)/8);
    /*
    if ((Map.width = 16) && (Map.height = 16)) current = 0;
    if ((Map.width = 16) && (Map.height = 24)) current = 1;
    if ((Map.width = 16) && (Map.height = 32)) current = 2;
    if ((Map.width = 16) && (Map.height = 40)) current = 3;
    if ((Map.width = 24) && (Map.height = 24)) current = 4;
    if ((Map.width = 24) && (Map.height = 32)) current = 5;
    if ((Map.width = 32) && (Map.height = 24)) current = 6;
    if ((Map.width = 32) && (Map.height = 32)) current = 7;
    if ((Map.width = 32) && (Map.height = 48)) current = 8;
    if ((Map.width = 40) && (Map.height = 40)) current = 9;
    if ((Map.width = 48) && (Map.height = 64)) current = 10;
    if ((Map.width = 64) && (Map.height = 24)) current = 11;
    if ((Map.width = 64) && (Map.height = 32)) current = 12;
    if ((Map.width = 64) && (Map.height = 48)) current = 13;

*/
   /*    QMessageBox    Errormsg;
    QString info = QString::number((int) Map.width());+"x"+QString::number((int) Map.height());
    Errormsg.critical(0,"Error",info);
    Errormsg.setFixedSize(500,200);*/

    bool ok;

    QString item = QInputDialog::getItem(this, tr("Select map size"),
                                         tr("Map width x hight = "), items, current, false, &ok);
    if (ok && !item.isEmpty())
    {

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

    showgridAct = new QAction(tr("Show grid"), this);
    showgridAct->setCheckable(true);
    showgridAct->setChecked(false);
    showgridAct->setStatusTip(tr("Show/Hide the grid"));
    connect(showgridAct,&QAction::triggered,this,&MainWindow::griddiag);

    showtilewindowAct = new QAction(tr("Show tile selection window"), this);
    showtilewindowAct->setCheckable(true);
    showtilewindowAct->setChecked(true);
    showtilewindowAct->setStatusTip(tr("Show/Hide the tile selection window"));
    connect(showtilewindowAct,&QAction::triggered,this,&MainWindow::tilewindowdiag);

    showunitwindowAct = new QAction(tr("Show unit selection window"), this);
    showunitwindowAct->setCheckable(true);
    showunitwindowAct->setChecked(true);
    showunitwindowAct->setStatusTip(tr("Show/Hide the unit selection window"));
    connect(showunitwindowAct,&QAction::triggered,this,&MainWindow::unitwindowdiag);

    mapresizeAct = new QAction(tr("&Resize map"),this);
    mapresizeAct->setStatusTip(tr("Change the size of the map"));
    connect(mapresizeAct,&QAction::triggered,this,&MainWindow::mapresizediag);


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
    editMenu->addAction(mapresizeAct);
    editMenu->addSeparator();
    editMenu->addAction(showgridAct);
    editMenu->addAction(showtilewindowAct);
    editMenu->addAction(showunitwindowAct);

    configMenu = menuBar()->addMenu(tr("&Settings"));
    configMenu->addAction(setPathAct);
    configMenu->addAction(setScaleFactorAct);
}




int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    MainWindow               window;


    if ((!Read_Config()) || (!Check_for_game_files()))
    {
        QMessageBox              Errormsg;
        Errormsg.warning(0,"","I cannot find the required game files in "+GameDir+"! Please reconfigure directories in the settings.");
        Errormsg.setFixedSize(500,200);

    }

    screenrect = app.primaryScreen()->geometry();
    window.move(screenrect.left(), screenrect.top());
    window.resize(screenrect.width()/2,screenrect.bottom()/2.5);
    window.show();

    return app.exec();
}


