/*
 * HL Editor Version Beta 1
 * by Jan Knipperts (Dragonsphere /DOSReloaded)
 *
 * A map editor for the game History Line 1914-1918 by BlueByte
 *
 * Version info BETA 1:
*  First beta version. All functions of the editor are roughly implemented.
*  There may still be bugs and tests on different systems are necessary.
*
*  Known issues:
*  - The Codes.dat of History Line contains further attributes for the individual maps that are not yet taken into account by this editor.
*    New maps are only added to the game as two-player maps.
*  - File access in the main program runs in the main program via Qt / QFile routines, but in some header files still via the standard C routines or fopen_s.
*    File and directory names are not case-sensitive. This would have to be adapted for Linux compatibility.
*  - No check whether passwords contain only letters, whether files are overwritten or the file name is correct, etc.
*
*/



#include "mainwindow.h"
#include <QApplication>
#include <QtWidgets>
#include <QLineEdit>
#include <QMessageBox>
#include <QInputDialog>
#include <QScrollArea>
#include <QMouseEvent>
#include "tilelist.h"
#include "unitlist.h"
#include "buildable.h"
#include "building.h"


//Global variables and constants:

QString                  GameDir;                           //Path to History Line 1914-1918 (read from config file)
QString                  Map_file;                          //String for user selected map file
QString                  MapDir       = "/MAP";             //Maps should be in the MAP sub directory of the game
QString                  Palette_name = "/00.PAL";          //Standard VGA Palette file of the game
QString                  Code_name    = "/CODES.DAT";        //File with the levelcodes
QString                  Partlib_S_name = "/LIB/PARTS.LIB";   //Game ressource files for summer tile graphics
QString                  Partdat_S_name = "/LIB/PARTS.DAT";
QString                  Partlib_W_name = "/LIB/PARTW.LIB";   //Game ressource files for winter tile graphics
QString                  Partdat_W_name = "/LIB/PARTW.DAT";
QString                  Unitlib_name = "/LIB/UNIT.LIB";
QString                  Unitdat_name = "/LIB/UNIT.DAT";
QString                  Cfg = "/CONFIG.CFG";               //Our config file
QString                  Title = "History Line 1914-1918 Editor by Jan Knipperts - Version BETA 1";


QImage                   MapImage;                          //I use a QImage as Screenbuffer to draw the map
QImage                   MapImageScaled;                    //Additional buffer for the scaled map image

QImage                   TileListImage;                     //Screenbuffer for the tile selection child window
QImage                   TileListImageScaled;               //Additional buffer for the scaled map image
QScrollArea              *tilescrollArea;                   //ScrollArea of the chils window
QWidget                  *tile_selection;                   //and a widget for it

QImage                   UnitListImage;                     //Same for the Unit selection window
QImage                   UnitListImageScaled;
QScrollArea              *unitscrollArea;
QWidget                  *unit_selection;

QImage                   BuildableImage;                    //..and the child window to define buildable units
QImage                   BuildableImageScaled;
QScrollArea              *buildablescrollArea;
QWidget                  *buildable;

QImage                   Building_Image;                    //..and the child window to define a buildings contents
QImage                   Building_Image_Scaled;
QScrollArea              *Building_ScrollArea;
QWidget                  *building_window;
QLineEdit*               RessourceEdit;

QRect                    screenrect;                        //A QRect to save the screen geometry and position windows accordingly.

int                      Scale_factor = 2;                  //Default scaling factor for the old VGA bitmaps
unsigned char            selected_tile = 0x00;              //Define "Plains" as default tile
unsigned char            selected_unit = 0xFF;              //No unit is selected by default
int                      selected_building = -1;            //No building is selected by default
bool                     Res_loaded = false;                //to check if bitmaps have already been loaded into memory
bool                     summer = true;                     //set summer as default season for map ressources
bool                     no_tilechange = false;
bool                     ocean_map = false;



//now include the code to load game ressources etc.

#include "Lib.h"
#include "fin.h"
#include "codes.h"
#include "shp.h"
#include "other.h"



//=================== Main Window  ==========================


MainWindow::MainWindow()
//Creates Main Window and adds a scroll area to display maps

{
    createActions();
    createMenus();
    setWindowTitle(Title);

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



void MainWindow::closeEvent(QCloseEvent *event)
//Own closeEvent handler to make sure allocated memory will be properly released

{
    Release_Buffers();
    event->accept();
}


void MainWindow::mousePressEvent(QMouseEvent *event)
//Handle mouse events on the main window
{

    if (event->button() == Qt::LeftButton)
    {

        if (Map.loaded == true)
        {

            int pos_x = scrollArea->horizontalScrollBar()->value();
            int pos_y = scrollArea->verticalScrollBar()->value();
            QPoint mouse_pos = scrollArea->mapFromParent(event->pos());
            mouse_pos = mouse_pos + QPoint(pos_x,pos_y);

            //Calc field position from mouse cords
            //Thanks to Amit Patel for this elegant solution of a pixel coordinates to hexagon coordinates algorithm!
            //Source: http://www-cs-students.stanford.edu/~amitp/Articles/GridToHex.html

            int halfsize = Tilesize/2;
            int mx = mouse_pos.x() / Scale_factor; //Let's leave out the scaling to make things easier
            int my = mouse_pos.y() / Scale_factor;
            int hy = my / halfsize;
            int hx = mx / (Tilesize - Tileshift);

            int diagonale[2][12] = {            //the x values of the diagonals of the hexagon
                    {7,6,6,5,4,4,3,3,2,1,1,0},
                    {0,1,1,2,3,3,4,4,5,6,6,7}
            };

            if( diagonale[(hy+hx)%2][my %halfsize] >= mx %(Tilesize - Tileshift) ) //We can use the y coordinate (modulo the half row height) as an index into the diagonal
                hx--;

            hy = ((hy-(hx%2))/2);

            if (hx < 0) hx = 0;  //Just to be save...
            if (hy < 0) hy = 0;


             if ((hx < (Map.width-1)) && (hy < (Map.height-1)))  //Is the field on the map?
            {

                int field_pos = (hy*Map.width)+hx;

                if (!no_tilechange)
                {

                    //When a building is demolished...
                    if (((Map.data[field_pos*2] == 0x01) ||
                         (Map.data[field_pos*2] == 0x02)) || // HQ
                        ((Map.data[field_pos*2] >= 0x0C) &&
                        (Map.data[field_pos*2] <= 0x11))) //Factory, Depot
                    {
                        Building_stat.num_buildings--;

                        if ((Map.data[field_pos*2] == 0x01) || (Map.data[field_pos*2] == 0x02))
                        {
                            if (Building_stat.num_HQ > 0)  Building_stat.num_HQ--;
                        }
                        if ((Map.data[field_pos*2] >= 0x0C) && (Map.data[field_pos*2] <= 0x0E))
                        {
                            if (Building_stat.num_F > 0)  Building_stat.num_F--;
                        }
                        if ((Map.data[field_pos*2] >= 0x0F) && (Map.data[field_pos*2] <= 0x11))
                        {
                            if (Building_stat.num_D > 0)  Building_stat.num_D--;
                        }
                    }

                    //When a building is be built...
                    if (((selected_tile == 0x01) || (selected_tile == 0x02)) ||
                        ((selected_tile >= 0x0C) && (selected_tile <= 0x11)))
                    {
                        Building_stat.num_buildings++;

                        if (Building_info != NULL)
                            Building_info = (Building_data_ext*) realloc(Building_info,Building_stat.num_buildings*sizeof(Building_data_ext));
                        else
                            Building_info = (Building_data_ext*) malloc(Building_stat.num_buildings*sizeof(Building_data_ext));

                        Building_info[Building_stat.num_buildings-1].Field = field_pos;

                        Building_info[Building_stat.num_buildings-1].Properties = (Building_data*) malloc(sizeof(Building_data));

                        switch (selected_tile)
                        {
                            case 0x01:
                            {
                                Building_stat.num_HQ++;
                                Building_info[Building_stat.num_buildings-1].Properties->Owner = 0;
                                Building_info[Building_stat.num_buildings-1].Properties->Type = 0;
                                Building_info[Building_stat.num_buildings-1].Properties->Index = Building_stat.num_HQ;
                                break;
                            }
                            case 0x02:
                            {
                                Building_stat.num_HQ++;
                                Building_info[Building_stat.num_buildings-1].Properties->Owner = 1;
                                Building_info[Building_stat.num_buildings-1].Properties->Type = 0;
                                Building_info[Building_stat.num_buildings-1].Properties->Index = Building_stat.num_HQ;
                                break;
                            }
                            case 0x0C:  //Factory neutral
                            {
                                Building_stat.num_F++;
                                Building_info[Building_stat.num_buildings-1].Properties->Owner = 2;  //neutral
                                Building_info[Building_stat.num_buildings-1].Properties->Type = 1;
                                Building_info[Building_stat.num_buildings-1].Properties->Index = Building_stat.num_F;
                                break;
                            }
                            case 0x0D:  //Factory german
                            {
                                Building_stat.num_F++;
                                Building_info[Building_stat.num_buildings-1].Properties->Owner = 0;  //german
                                Building_info[Building_stat.num_buildings-1].Properties->Type = 1;
                                Building_info[Building_stat.num_buildings-1].Properties->Index = Building_stat.num_F;
                                break;
                            }
                            case 0x0E:  //Factory french
                            {
                                Building_stat.num_F++;
                                Building_info[Building_stat.num_buildings-1].Properties->Owner = 1;  //french
                                Building_info[Building_stat.num_buildings-1].Properties->Type = 1;
                                Building_info[Building_stat.num_buildings-1].Properties->Index = Building_stat.num_F;
                                break;
                            }
                            case 0x0F:  //Depot neutral
                            {
                                Building_stat.num_D++;
                                Building_info[Building_stat.num_buildings-1].Properties->Owner = 2;  //neutral
                                Building_info[Building_stat.num_buildings-1].Properties->Type = 2;
                                Building_info[Building_stat.num_buildings-1].Properties->Index = Building_stat.num_D;
                                break;
                            }
                            case 0x10:  //Depot german
                            {
                                Building_stat.num_D++;
                                Building_info[Building_stat.num_buildings-1].Properties->Owner = 0;  //german
                                Building_info[Building_stat.num_buildings-1].Properties->Type = 2;
                                Building_info[Building_stat.num_buildings-1].Properties->Index = Building_stat.num_D;
                                break;
                            }
                            case 0x11:  //Depot french
                            {
                                Building_stat.num_D++;
                                Building_info[Building_stat.num_buildings-1].Properties->Owner = 1;  //french
                                Building_info[Building_stat.num_buildings-1].Properties->Type = 2;
                                Building_info[Building_stat.num_buildings-1].Properties->Index = Building_stat.num_D;
                                break;
                            }
                        }

                        Building_info[Building_stat.num_buildings-1].Properties->Resources = 0;
                        Building_info[Building_stat.num_buildings-1].Properties->Units[0] = 0xFF;
                        Building_info[Building_stat.num_buildings-1].Properties->Units[1] = 0xFF;
                        Building_info[Building_stat.num_buildings-1].Properties->Units[2] = 0xFF;
                        Building_info[Building_stat.num_buildings-1].Properties->Units[3] = 0xFF;
                        Building_info[Building_stat.num_buildings-1].Properties->Units[4] = 0xFF;
                        Building_info[Building_stat.num_buildings-1].Properties->Units[5] = 0xFF;
                        Building_info[Building_stat.num_buildings-1].Properties->Units[6] = 0xFF;
                        Building_info[Building_stat.num_buildings-1].Properties->Unknown = 0;

                    }

                    Map.data[field_pos*2] = (unsigned char) selected_tile;
                }


                if ((Map.data[(field_pos*2)+1] == 0x2C) ||
                    (Map.data[(field_pos*2)+1] == 0x2D) ||
                    (Map.data[(field_pos*2)+1] == 0x34) ||
                    (Map.data[(field_pos*2)+1] == 0x35) ||
                    (Map.data[(field_pos*2)+1] == 0x3E) ||
                    (Map.data[(field_pos*2)+1] == 0x3F)) //Transport)
                    Building_stat.num_buildings--;

                if ((selected_unit == 0x2C) ||
                    (selected_unit == 0x2D) ||
                    (selected_unit == 0x34) ||
                    (selected_unit == 0x35) ||
                    (selected_unit == 0x3E) ||
                    (selected_unit == 0x3F)) //Transport)
                {
                    Building_stat.num_buildings++;
                    if (Building_info != NULL)
                        Building_info = (Building_data_ext*) realloc(Building_info,Building_stat.num_buildings*sizeof(Building_data_ext));
                    else
                        Building_info = (Building_data_ext*) malloc(Building_stat.num_buildings*sizeof(Building_data_ext));

                    Building_info[Building_stat.num_buildings-1].Field = field_pos;
                    Building_info[Building_stat.num_buildings-1].Properties = (Building_data*) malloc(sizeof(Building_data));

                    Building_stat.num_T++;
                    if (selected_unit % 2 == 0)
                        Building_info[Building_stat.num_buildings-1].Properties->Owner = 1;  //french
                    else
                        Building_info[Building_stat.num_buildings-1].Properties->Owner = 0;  //german

                    Building_info[Building_stat.num_buildings-1].Properties->Type = 3;
                    Building_info[Building_stat.num_buildings-1].Properties->Index = Building_stat.num_T;
                    Building_info[Building_stat.num_buildings-1].Properties->Resources = 0;
                    Building_info[Building_stat.num_buildings-1].Properties->Units[0] = 0xFF;
                    Building_info[Building_stat.num_buildings-1].Properties->Units[1] = 0xFF;
                    Building_info[Building_stat.num_buildings-1].Properties->Units[2] = 0xFF;
                    Building_info[Building_stat.num_buildings-1].Properties->Units[3] = 0xFF;
                    Building_info[Building_stat.num_buildings-1].Properties->Units[4] = 0xFF;
                    Building_info[Building_stat.num_buildings-1].Properties->Units[5] = 0xFF;
                    Building_info[Building_stat.num_buildings-1].Properties->Units[6] = 0xFF;
                    Building_info[Building_stat.num_buildings-1].Properties->Unknown = 0;
                }

                Map.data[(field_pos*2)+1] = (unsigned char) selected_unit;
                Redraw_Field(hx,hy,selected_tile,selected_unit);

                MapImageScaled = MapImage.scaled(MapImage.width()*Scale_factor,MapImage.height()*Scale_factor); //Create a scaled version of it
                if(showgridAct->isChecked()) ShowGrid();  //redraw the grid if enabled
                Draw_Hexagon(hx,hy,QPen(Qt::red, 1),&MapImageScaled,true,true); //redraw the frame


                QLabel *imageLabel = new QLabel;     //Create a scroll area to display the map
                imageLabel->setPixmap(QPixmap::fromImage(MapImageScaled));
                scrollArea->setWidget(imageLabel);

                scrollArea->horizontalScrollBar()->setValue(pos_x); //Reset the scrollArea to last position
                scrollArea->verticalScrollBar()->setValue(pos_y);

            }
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


            if (((Map.data[field_pos*2] == 0x01) ||
                 (Map.data[field_pos*2] == 0x02)) || // HQ
                ((Map.data[field_pos*2] >= 0x0C) &&
                 (Map.data[field_pos*2] <= 0x11)) || //Fabrik, Depot
                ((Map.data[(field_pos*2)+1] == 0x2C) ||
                 (Map.data[(field_pos*2)+1] == 0x2D) ||
                 (Map.data[(field_pos*2)+1] == 0x34) ||
                 (Map.data[(field_pos*2)+1] == 0x35) ||
                 (Map.data[(field_pos*2)+1] == 0x3E) ||
                 (Map.data[(field_pos*2)+1] == 0x3F))) //Transport
            {

                selected_building = Get_Buildings_info(field_pos);

                if (building_window == NULL)
                  Create_building_configuration_window(Get_Buildings_info(field_pos));
                else
                {
                  building_window->close();
                  Create_building_configuration_window(Get_Buildings_info(field_pos));
                }
            }

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



void MainWindow::newFile_diag()
{
    if (!Res_loaded)
    {
        if (Load_Ressources() != 0)
        {
            QMessageBox              Errormsg;
            Errormsg.critical(0,"Error","Failed to load bitmaps from the game!");
            Errormsg.setFixedSize(500,200);
            return;
        }
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

        Map.loaded = true; //Blank map loaded successfully ;)

        if (!MapImage.isNull()) MapImage = QImage(); //Release mem for the last used image
        if (!MapImageScaled.isNull()) MapImageScaled = QImage(); //Release mem for the last used scaled image
        MapImage = QImage(((Map.width/2)*Tilesize)+(((Map.width/2)-1)*Tileshift),((Map.height-1)*Tilesize)+(Tilesize/2), QImage::Format_RGB16); //Create a new QImage object for the map image
        MapImage.fill(Qt::transparent);

        Draw_Map(); //and draw the map to it
        MapImageScaled = MapImage.scaled(MapImage.width()*Scale_factor,MapImage.height()*Scale_factor); //Create a scaled version of it        
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

        Building_stat.num_HQ = 0;
        Building_stat.num_F = 0;
        Building_stat.num_D = 0;
        Building_stat.num_T = 0;
        Building_stat.num_buildings = 0;
        if (Building_info != NULL) free(Building_info);
        if (SHP.buildings != NULL) free(SHP.buildings);
        Building_info = (Building_data_ext*) malloc(sizeof(Building_data_ext));
    }
}

void MainWindow::open_diag()
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
            Map.loaded = true;
            if(showgridAct->isChecked()) ShowGrid();  //redraw the grid if enabled
            QLabel *imageLabel = new QLabel;     //Create a scroll area to display the map
            imageLabel->setPixmap(QPixmap::fromImage(MapImageScaled));

            scrollArea->setWidget(imageLabel);



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

void MainWindow::save_diag()
{
    if (Map.loaded == true)
    {
        Map_file = QFileDialog::getSaveFileName(this,
                                                tr("Save History Line 1914-1918 map file"),MapDir, tr("HL map files (*.fin)"));
        if (Map_file != "")
        {
            if (Save_Mapdata(Map_file.toStdString().data()) != 0)
            {
              QMessageBox              Errormsg;
              Errormsg.warning(0,"","I cannot save the map data in "+Map_file);
              Errormsg.setFixedSize(500,200);
            }
            QString SHPfile;
            SHPfile = Map_file;
            SHPfile.replace(".fin",".shp").replace(".FIN",".SHP");
            if (Create_shp(SHPfile.toStdString().data()) != 0)
            {
                QMessageBox              Errormsg;
                Errormsg.warning(0,"","I cannot save the building data in "+SHPfile);
                Errormsg.setFixedSize(500,200);
            }
        }
    }
}



void MainWindow::grid_diag()
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


void MainWindow::tilewindow_diag()
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

void MainWindow::unitwindow_diag()
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


void MainWindow::setPath_diag()
{

    QDir           dir;


    GameDir = QFileDialog::getExistingDirectory(this, tr("Please select the directory of Historyline 1914-1918"),
                                                GameDir,
                                                QFileDialog::ShowDirsOnly
                                                    | QFileDialog::DontResolveSymlinks);

    if (!Check_for_game_files())
    {
        QMessageBox              Errormsg;
        Errormsg.warning(0,"","I cannot find the required game files in the selected directory!");
        Errormsg.setFixedSize(500,200);
    }
    else
    {

        QFile cfgFile(dir.currentPath()+Cfg);
        cfgFile.open(QIODevice::WriteOnly);

        if (!cfgFile.isOpen())
        {
            QMessageBox              Errormsg;
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

void MainWindow::setScale_diag()
{

    bool ok;
    Scale_factor = QInputDialog::getInt(
        this,
        tr("Scaling of VGA bitmaps:"),
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


void MainWindow::add_diag()
{
    QString Codefile;

    if (Map.loaded == true)
    {
        bool ok;
        QString levelcode = QInputDialog::getText(this, tr("QInputDialog::getText()"),
                                         tr("Enter a levelcode for your map (5 letters):"), QLineEdit::Normal,
                                        "", &ok);
        if (ok && !levelcode.isEmpty())
        {
            if (levelcode.length() != 5)
            {
              QMessageBox   Errormsg;
              Errormsg.critical(0,"","Code must be five letters to work with the game.");
              Errormsg.setFixedSize(500,200);
              return;
            }

            if (Levelcode_exists(levelcode))
            {
              QMessageBox   Errormsg;
              Errormsg.critical(0,"","Level already exists. Please choose another code for it.");
              Errormsg.setFixedSize(500,200);
              return;
            }

            QDir Map_dir(MapDir);
            QStringList maps = Map_dir.entryList(QStringList() << "*.fin" << "*.FIN",QDir::Files);

            for(int i = maps.size()-1; i >= 0; i--)
            {
                if (!maps[i][0].isDigit())
                  maps.removeAt(i);
            }

            if (maps.size()-1 >= 99 )
            {
                QMessageBox   Errormsg;
                Errormsg.critical(0,"","There are too many maps in the directory. The game can handle a maximum of 99.");
                Errormsg.setFixedSize(500,200);
                return;
            }

            int filenumber = QString((QString) maps[maps.size()-1][0]+maps[maps.size()-1][1]).toInt();
            filenumber++;
            Map_file = QString::number(filenumber);
            Map_file = Map_file+".fin" ;
            Map_file = MapDir+"/"+Map_file;
            Map_file.replace("/'", "\\'");
            if (Save_Mapdata(Map_file.toStdString().data()) != 0) //Create new .fin file for this map.
            {
                QMessageBox   Errormsg;
                Errormsg.critical(0,"","Failed to create "+Map_file);
                Errormsg.setFixedSize(500,200);
                return;
            }

            Codefile = (GameDir + Code_name); //Create a C style filename for use of stdio
            Codefile.replace("/'", "\\'");

            if (Add_map(Codefile.toStdString().data(), levelcode) != 0)
            {
                QMessageBox   Errormsg;
                Errormsg.critical(0,"","Can't write data for the new map to CODES.DAT");
                Errormsg.setFixedSize(500,200);
                return;
            }


            QMessageBox Errormsg;
            Errormsg.information(0,"Map added","Added "+Map_file+" with code "+levelcode);
            Errormsg.setFixedSize(500,200);
        }
    }
    else
    {
        QMessageBox Errormsg;
        Errormsg.warning(0,"","Please load or create a map first.");
        Errormsg.setFixedSize(500,200);
    }
}


void MainWindow::map_resize_diag()
{

    if  (Map.loaded == true)
    {
    int             current, width, height,x,y,o,o1;
    unsigned char*  data;
    bool            ok;

    QStringList     items;

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


    if ((Map.width == 16) && (Map.height == 16)) current = 0;
    if ((Map.width == 16) && (Map.height == 24)) current = 1;
    if ((Map.width == 16) && (Map.height == 32)) current = 2;
    if ((Map.width == 16) && (Map.height == 40)) current = 3;
    if ((Map.width == 24) && (Map.height == 24)) current = 4;
    if ((Map.width == 24) && (Map.height == 32)) current = 5;
    if ((Map.width == 32) && (Map.height == 24)) current = 6;
    if ((Map.width == 32) && (Map.height == 32)) current = 7;
    if ((Map.width == 32) && (Map.height == 48)) current = 8;
    if ((Map.width == 40) && (Map.height == 40)) current = 9;
    if ((Map.width == 48) && (Map.height == 64)) current = 10;
    if ((Map.width == 64) && (Map.height == 24)) current = 11;
    if ((Map.width == 64) && (Map.height == 32)) current = 12;
    if ((Map.width == 64) && (Map.height == 48)) current = 13;


    QString item = QInputDialog::getItem(this, tr("Select map size"),
                                         tr("Map width x height = "), items, current, false, &ok);
    if (ok && !item.isEmpty())
    {

        if (item == "16x16"){ width = 16;height = 16;}
        if (item == "16x24"){ width = 16;height = 24;}
        if (item == "16x32"){ width = 16;height = 32;}
        if (item == "16x40"){ width = 16;height = 40;}
        if (item == "24x24"){ width = 24;height = 24;}
        if (item == "24x32"){ width = 24;height = 32;}
        if (item == "32x24"){ width = 32;height = 24;}
        if (item == "32x32"){ width = 32;height = 32;}
        if (item == "32x48"){ width = 32;height = 48;}
        if (item == "40x40"){ width = 40;height = 40;}
        if (item == "48x64"){ width = 48;height = 64;}
        if (item == "64x24"){ width = 64;height = 24;}
        if (item == "64x32"){ width = 64;height = 32;}
        if (item == "64x48"){ width = 64;height = 48;}


        if ((data = (unsigned char*)malloc(((width+1) * (height+1) * 2))) == NULL)
        {
            QMessageBox  Errormsg;
            Errormsg.critical(0,"Error","Memory allocation error!");
            Errormsg.setFixedSize(500,200);
            return;
        }
        else
        {
            o = 0;
            o1 = 0;
            for (y = 1; y <= height; y++)
            {
              o1 = ((y-1)*Map.width)*2;
                for (x = 1; x <= width; x++)
                {
                    if ((x == width) || (y == height))
                    {
                        data[o] =  (unsigned char) 0xAE;
                        data[o+1] =  (unsigned char) 0xFF;
                    }
                    else
                    {
                        if  ((x >= Map.width) || (y >= Map.height))
                        {
                            data[o] =  (unsigned char) 0x00;
                            data[o+1] =  (unsigned char) 0xFF;
                        }
                        else
                        {
                            data[o] = (unsigned char) Map.data[o1];
                            data[o+1] = (unsigned char) Map.data[o1+1];
                            o1 = o1 + 2;
                        }
                    }
                    o = o + 2;
                }

            }
       }

        if  (Map.data != NULL) free(Map.data); //Release old data
        Map.width = width; //Set new width, height and size
        Map.height = height;
        Map.data_size = ((Map.width+1) * (Map.height+1)) * 2;
        Map.data = data; //Let Map.data point to new buffer
        MapImage = QImage(((Map.width/2)*Tilesize)+(((Map.width/2)-1)*Tileshift),((Map.height-1)*Tilesize)+(Tilesize/2), QImage::Format_RGB16); //Create a new QImage object for the map image
        MapImage.fill(Qt::transparent);
        Draw_Map(); //and draw the map to it
        MapImageScaled = MapImage.scaled(MapImage.width()*Scale_factor,MapImage.height()*Scale_factor); //Create a scaled version of it
        Map.loaded = true;
        if(showgridAct->isChecked()) ShowGrid();  //redraw the grid if enabled

        QLabel *imageLabel = new QLabel;     //Create a scroll area to display the map
        imageLabel->setPixmap(QPixmap::fromImage(MapImageScaled));
        scrollArea->setWidget(imageLabel);


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
    else
    {
    QMessageBox Errormsg;
    Errormsg.warning(0,"","Please load or create a map first.");
    Errormsg.setFixedSize(500,200);
    }
}

void MainWindow::season_diag()
{
    QMessageBox              Errormsg;
    QString                  C_Filename1;
    QString                  C_Filename2;

    if (summer == true)   //Change to winter
    {
        C_Filename1 = (GameDir + Partlib_W_name); //Create C style filenames for use of stdio
        C_Filename1.replace("/", "\\");
        C_Filename2 = (GameDir + Partdat_W_name);
        C_Filename2.replace("/", "\\");
        summer = false;
    }
    else
    {
        C_Filename1 = (GameDir + Partlib_S_name); //Create C style filenames for use of stdio
        C_Filename1.replace("/", "\\");
        C_Filename2 = (GameDir + Partdat_S_name);
        C_Filename2.replace("/", "\\");
        summer = true;
    }

    if (Partlib.data != NULL) free(Partlib.data);

    if (Load_Part_files(C_Filename1.toStdString().data(),C_Filename2.toStdString().data()) != 0)
    {
        Errormsg.critical(0,"Error","Faild to load summer/winter graphics from the game!");
        Errormsg.setFixedSize(500,200);
        return;
    }

    if (Map.loaded == true)
    {
        MapImage.fill(Qt::transparent);
        Draw_Map(); //redraw the mapimage

        MapImageScaled = MapImage.scaled(MapImage.width()*Scale_factor,MapImage.height()*Scale_factor); //Create a scaled version of it
        if(showgridAct->isChecked()) ShowGrid();  //redraw the grid if enabled
        QLabel *imageLabel = new QLabel;     //Create a scroll area to display the map
        imageLabel->setPixmap(QPixmap::fromImage(MapImageScaled));
        scrollArea->setWidget(imageLabel);

        if (showtilewindowAct->isChecked() == true)  //Update Tile selection
        {
        TileListImage.fill(Qt::transparent);
        int tx = 0;
        int ty = 0;

        for (int tc = 0; tc < Num_Parts; tc++)
        {
            Draw_Part(tx*Tilesize,ty*Tilesize,tc,&TileListImage); //Draw the bitmap
            tx++;
            if (tx == 10)
            {
                tx = 0;
                ty++;
            }
        }

        TileListImageScaled = TileListImage.scaled(TileListImage.width()*Scale_factor,TileListImage.height()*Scale_factor); //Create a scaled version of it
        Draw_Hexagon(0,0,QPen(Qt::red, 1),&TileListImageScaled,false,true);
        selected_tile = 0;
        QLabel *label = new QLabel();
        QHBoxLayout *layout = new QHBoxLayout();
        label->setPixmap(QPixmap::fromImage(TileListImageScaled));
        tilescrollArea->setWidget(label);
        layout->addWidget(tilescrollArea);
        tile_selection->update();
        }
    }

}

void MainWindow::flood_diag()
{
    QMessageBox              Errormsg;
    int                      x,y,offset;
    field_info               fielddata;

    if (Map.loaded == true)
    {
        if (!ocean_map)
            ocean_map = true;
        else
            ocean_map = false;




        offset = 0;
        for (y = 0; y < Map.height; y++)
        {
            for (x = 0; x < Map.width; x++)
            {
                memcpy(&fielddata, Map.data + offset, sizeof(fielddata));
                if (ocean_map)
                {
                    if (fielddata.Part == 0x00)
                    {
                        fielddata.Part = 0x2F; //Change plains to deep water
                        memcpy(Map.data + offset,&fielddata, sizeof(fielddata));
                     }
                }
                else
                {
                    if (fielddata.Part == 0x2F)
                    {
                       fielddata.Part = 0x00; //Change deep water to plains
                       memcpy(Map.data + offset,&fielddata, sizeof(fielddata));
                    }
                }
                offset = offset + sizeof(Field);
            }
        }



        MapImage.fill(Qt::transparent);
        Draw_Map(); //redraw the mapimage

        MapImageScaled = MapImage.scaled(MapImage.width()*Scale_factor,MapImage.height()*Scale_factor); //Create a scaled version of it
        if(showgridAct->isChecked()) ShowGrid();  //redraw the grid if enabled
        QLabel *imageLabel = new QLabel;     //Create a scroll area to display the map
        imageLabel->setPixmap(QPixmap::fromImage(MapImageScaled));
        scrollArea->setWidget(imageLabel);

    }
    else
    {
        QMessageBox Errormsg;
        Errormsg.warning(0,"","Please load or create a map first.");
        Errormsg.setFixedSize(500,200);
    }

}


void MainWindow::buildable_units_diag()
{
    if (Map.loaded == true)
    {
        if (buildable == NULL)
            Create_buildable_units_window();
        else
            buildable->show();
    }
    else
    {
        QMessageBox Errormsg;
        Errormsg.warning(0,"","Please load or create a map first.");
        Errormsg.setFixedSize(500,200);
    }
}

void MainWindow::createActions()
{
    newAct = new QAction(tr("&New"), this);
    newAct->setShortcuts(QKeySequence::New);
    newAct->setStatusTip(tr("Create a new map"));
    connect(newAct, &QAction::triggered, this, &MainWindow::newFile_diag);

    openAct = new QAction(tr("&Open..."), this);
    openAct->setShortcuts(QKeySequence::Open);
    openAct->setStatusTip(tr("Open an existing map"));
    connect(openAct, &QAction::triggered, this, &MainWindow::open_diag);

    saveAct = new QAction(tr("&Save"), this);
    saveAct->setShortcuts(QKeySequence::Save);
    saveAct->setStatusTip(tr("Save the map to disk"));
    connect(saveAct, &QAction::triggered, this, &MainWindow::save_diag);

    addtogameAct = new QAction(tr("&Add map to game"), this);
    addtogameAct->setStatusTip(tr("Adds your map to the game"));
    connect(addtogameAct, &QAction::triggered, this, &MainWindow::add_diag);

    exitAct = new QAction(tr("E&xit"), this);
    exitAct->setShortcuts(QKeySequence::Quit);
    exitAct->setStatusTip(tr("Exit HL Editor"));
    connect(exitAct, &QAction::triggered, this, &QWidget::close);

    showgridAct = new QAction(tr("Show grid"), this);
    showgridAct->setCheckable(true);
    showgridAct->setChecked(false);
    showgridAct->setStatusTip(tr("Show/Hide the grid"));
    connect(showgridAct,&QAction::triggered,this,&MainWindow::grid_diag);

    showtilewindowAct = new QAction(tr("Show tile selection window"), this);
    showtilewindowAct->setCheckable(true);
    showtilewindowAct->setChecked(true);
    showtilewindowAct->setStatusTip(tr("Show/Hide the tile selection window"));
    connect(showtilewindowAct,&QAction::triggered,this,&MainWindow::tilewindow_diag);

    showunitwindowAct = new QAction(tr("Show unit selection window"), this);
    showunitwindowAct->setCheckable(true);
    showunitwindowAct->setChecked(true);
    showunitwindowAct->setStatusTip(tr("Show/Hide the unit selection window"));
    connect(showunitwindowAct,&QAction::triggered,this,&MainWindow::unitwindow_diag);

    mapresizeAct = new QAction(tr("&Resize map"),this);
    mapresizeAct->setStatusTip(tr("Change the size of the map"));
    connect(mapresizeAct,&QAction::triggered,this,&MainWindow::map_resize_diag);

    changeseasonAct = new QAction(tr("Toggle summer/winter"),this);
    changeseasonAct->setStatusTip(tr("Map plays in summer or in winter"));
    connect(changeseasonAct,&QAction::triggered,this,&MainWindow::season_diag);

    floodAct = new QAction(tr("Flood/dry map"),this);
    floodAct->setStatusTip(tr("Ocean or ground map?"));
    connect(floodAct,&QAction::triggered,this,&MainWindow::flood_diag);

    buildableunitsAct = new QAction(tr("Set buildable units"),this);
    buildableunitsAct->setStatusTip(tr("Which units can be built in factories?"));
    connect(buildableunitsAct,&QAction::triggered,this,&MainWindow::buildable_units_diag);

    setPathAct = new QAction(tr("&Game path"),this);
    setPathAct->setStatusTip(tr("Set path to game resources"));
    connect(setPathAct,&QAction::triggered,this,&MainWindow::setPath_diag);

    setScaleFactorAct = new QAction(tr("&Scale factor"),this);
    setScaleFactorAct->setStatusTip(tr("Scaler used to enlarge/enhance low resolution bitmaps."));
    connect(setScaleFactorAct,&QAction::triggered,this,&MainWindow::setScale_diag);
}


void MainWindow::createMenus()
{
    fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(newAct);
    fileMenu->addAction(openAct);
    fileMenu->addAction(saveAct);
    fileMenu->addAction(addtogameAct);
    fileMenu->addSeparator();
    fileMenu->addAction(exitAct);

    editMenu =  menuBar()->addMenu(tr("&Edit"));
    editMenu->addAction(mapresizeAct);
    editMenu->addAction(changeseasonAct);
    editMenu->addAction(floodAct);
    editMenu->addAction(buildableunitsAct);
    editMenu->addSeparator();
    editMenu->addAction(showgridAct);
    editMenu->addAction(showtilewindowAct);
    editMenu->addAction(showunitwindowAct);

    configMenu = menuBar()->addMenu(tr("&Settings"));
    configMenu->addAction(setPathAct);
    configMenu->addAction(setScaleFactorAct);
}



//========================== Event handling for child windows =================================


void tilelistwindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        int pos_x = tilescrollArea->horizontalScrollBar()->value();
        int pos_y = tilescrollArea->verticalScrollBar()->value();
        TileListImageScaled = TileListImage.scaled(TileListImage.width()*Scale_factor,TileListImage.height()*Scale_factor); //Restore original image
        int fx = (event->pos().x()+pos_x) / (Tilesize*Scale_factor); // Calc field position from mouse cords
        int fy = (event->pos().y()+pos_y) / (Tilesize*Scale_factor);

        if (((fy*10)+fx) <= Num_Parts-1)
        {
            selected_tile = (fy*10)+fx;
            Draw_Hexagon(fx,fy,QPen(Qt::red, 1),&TileListImageScaled,false,true); //Draw the frame
        }

        no_tilechange = false;

        QLabel *label = new QLabel();
        label->setPixmap(QPixmap::fromImage(TileListImageScaled));

        tilescrollArea->setWidget(label);
        tile_selection->update();
        tilescrollArea->horizontalScrollBar()->setValue(pos_x); //Reset the scrollArea to last position
        tilescrollArea->verticalScrollBar()->setValue(pos_y);
    }

    if (event->button() == Qt::RightButton)
    {
        int pos_x = tilescrollArea->horizontalScrollBar()->value();
        int pos_y = tilescrollArea->verticalScrollBar()->value();
        TileListImageScaled = TileListImage.scaled(TileListImage.width()*Scale_factor,TileListImage.height()*Scale_factor); //Restore original image
        QLabel *label = new QLabel();
        label->setPixmap(QPixmap::fromImage(TileListImageScaled));

        selected_tile = 0xFF;
        no_tilechange = true;

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

    if (event->button() == Qt::RightButton)
    {
        int pos_x = unitscrollArea->horizontalScrollBar()->value();
        int pos_y = unitscrollArea->verticalScrollBar()->value();
        UnitListImageScaled = UnitListImage.scaled(UnitListImage.width()*Scale_factor,UnitListImage.height()*Scale_factor); //Restore original image

        selected_unit = 0xFF;     //No unit selected

        QLabel *label = new QLabel();
        label->setPixmap(QPixmap::fromImage(UnitListImageScaled));  //update the image

        unitscrollArea->setWidget(label);
        unit_selection->update();                           //Update the window contents
        unitscrollArea->horizontalScrollBar()->setValue(pos_x); //Reset the scrollArea to last position
        unitscrollArea->verticalScrollBar()->setValue(pos_y);
    }
}


void buildablewindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        int pos_x = buildablescrollArea->horizontalScrollBar()->value();
        int pos_y = buildablescrollArea->verticalScrollBar()->value();

        int fx = (event->pos().x()+pos_x) / (Tilesize*Scale_factor); // Calc field position from mouse cords
        int fy = (event->pos().y()+pos_y) / (Tilesize*Scale_factor);

        int unit = ((fy*10)+fx);     //Calc correct unit number


        if (unit <= Num_Units)
        {
            if (SHP.can_be_built[unit] == 0)
                SHP.can_be_built[unit] = 1;
            else
                SHP.can_be_built[unit] = 0;

        }


        int tx = 0;
        int ty = 0;

        for (int tc = 0; tc < Num_Units; tc++)
        {
            if (SHP.can_be_built[tc] == 0)
                Draw_Unit(tx*Tilesize,ty*Tilesize,tc*6,1,&BuildableImage);
            else
                Draw_Unit(tx*Tilesize,ty*Tilesize,tc*6,3,&BuildableImage);


            tx++;
            if (tx == 10)
            {
                tx = 0;
                ty++;
            }
        }


        BuildableImageScaled = BuildableImage.scaled(BuildableImage.width()*Scale_factor,BuildableImage.height()*Scale_factor); //Create a scaled version of it


        QLabel *label = new QLabel();
        label->setPixmap(QPixmap::fromImage( BuildableImageScaled));  //update the image

        buildablescrollArea->setWidget(label);
        buildable->update();                           //Update the window contents
        buildablescrollArea->horizontalScrollBar()->setValue(pos_x); //Reset the scrollArea to last position
        buildablescrollArea->verticalScrollBar()->setValue(pos_y);
    }
}




void buildingwindow::mousePressEvent(QMouseEvent *event)
{
    if ((event->button() == Qt::LeftButton) && (selected_building != -1))
    {
        QPoint mouse_pos = Building_ScrollArea->mapFromParent(event->pos());

        //Is the mouse on the ScrollArea (The list of units in the building?)
        if ((mouse_pos.x() >= 0) &&  (mouse_pos.y() >= 0) &&
            (mouse_pos.x() < Building_Image_Scaled.width()) && (mouse_pos.y() < Building_Image_Scaled.height()))
        {
            int fx = (mouse_pos.x() / Scale_factor) / Tilesize;

            Building_info[selected_building].Properties->Units[fx] = selected_unit/2;

            Draw_Unit(fx*Tilesize, 0,(Building_info[selected_building].Properties->Units[fx] * 6) , Building_info[selected_building].Properties->Owner+1, &Building_Image);
            Building_Image_Scaled = Building_Image.scaled(Building_Image.width()*Scale_factor,Building_Image.height()*Scale_factor); //Create a scaled version of it
            for (int i = 0; i < 7; i++)
            Draw_Hexagon(i,0,QPen(Qt::white, 1),&Building_Image_Scaled,false,true);

            QLabel *bitmaplabel = new QLabel();
            bitmaplabel->setPixmap(QPixmap::fromImage(Building_Image_Scaled));
            Building_ScrollArea->setWidget(bitmaplabel);
            Building_ScrollArea->update();
            building_window->update();
        }
    }

}

void buildingwindow::closeEvent(QCloseEvent *event)
{  
    if (selected_building != -1)
        Building_info[selected_building].Properties->Resources = RessourceEdit->text().toInt();

    event->accept();
}


//========================== Main program ================================



int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    MainWindow               window;

    screenrect = app.primaryScreen()->geometry();   //Save screen geometry for window positioning
    window.resize(screenrect.width()/2, screenrect.height() / 2);
    window.move(screenrect.left(),screenrect.top());
    window.show();

    if ((!Read_Config()) || (!Check_for_game_files()))  //Check for config and game files first
    {
        QMessageBox   Errormsg;
        Errormsg.warning(0,"","I cannot find the required game files! Please reconfigure directories in the settings.");
        Errormsg.setFixedSize(500,200);
    }


    return app.exec();
}


