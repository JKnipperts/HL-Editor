/*
 * HL Editor - 1st Release Candidate (Beta 1.7)
 * by Jan Knipperts (Dragonsphere /DOSReloaded)
 *
 * A map editor for the game History Line 1914-1918 by BlueByte
 *
 * Version info:
*  Beta version! Errors and bugs may still occur and the program has not yet been sufficiently tested on various systems.
*
*  Changes to BETA 1.6
*  - Complete buildings can be places by selecting the entry and right clicking on the map.
*  - fixed some dialog titles, removed help icons etc.
*  - corrected handling of "end of campaign" marker
*  - The name of the selected unit is now also displayed in the window for setting the buildable units
*  - existing maps can be removed from the game
*  - Warning if single parts of factories or depots are placed
*
*  Changes to BETA 1.5
*  - Fixed a bug in the display of german "ü"-Umlaut in the unit name.
*  - Fixed German units alignement in the selection window
*  - Now the terrain under a unit can also be changed without deleting it. Units can now be deleted by double-clicking on them.
*  - Resources and units in the building are now retained when a building is replaced by another building.
*  - Fixed a bug in the assignment of building data when buildings were replaced by others.
*  - Maps can now also be loaded via their level code
*  - The season and map type for in-game maps is read from the Codes.dat file.
*    For cards that have not yet been added to the game, a temp file is created to save the season and card type.
*  - It is now possible to create singelplayer maps with computer opponents.
*  - Fixed setting of the "End of campaign"-marker when adding new maps to the game.
*
*  Changes to BETA 1.4
*  - Map info / Statistics added
*  - Fixed a bug that confused the assignment of fields to buildings when changing the map size
*  - A warning is issued when adding to the game and when calling up the statistics if more terrain graphics have been used than the game can process.
*  - A warning is issued if units are placed on fields that are unsuitable for them.
*  - These warnings can also be switched off under Settings.
*
*  Changes to BETA 1.3
*  - Fixed a bug in TPWM_unpack that caused writing attempts outside the buffer in some cases.
*  - When creating a new map, you will now be asked whether it should be created as an ocean or a land map.
*  - Optimized accuracy when selecting or placing fields or units with the mouse
*  - For Linux compatibility, a separate check is now also carried out for lower-case file names when loading the game files.
*
*  Changes to BETA 1.2:
*  - Units in buildings can now be removed with the right mouse button
*  - Fields for units in buildings are now rectangular to make them clearer
*  - Fixed a bug that prevented changing the map size in some cases
*  - Improved handling of the cancel button for QFileDialogs
*  - If the scaling factor is changed, the child windows are now also redrawn accordingly
*  - Reference to beta version and greetings added
*
*  Changes to BETA 1.1:
*  - Name of selected unit is shown in the unit selection window
*  - Fixed bug in handling of not saved transport units in the original game files
*  - New level codes are being checked
*  - Query whether unsaved changes should be saved when exiting or reloading
*  - some small improvements in the code
*  - New messages. For example, an attempt is made to save and no map has been loaded or created beforehand
*
*  Changes to BETA 1.0:
*  - Fixed bug: When importing a map into the game, SHP data was not saved correctly.
*  - Fixed bug: Resource income from buildings is now displayed correctly.
*  - The right mouse button can now also be used to select a field without placing anything directly on it.
*  - Minor optimizations to the code
*  - Fixed "Using QCharRef with an index pointing outside the valid range of a QString." warning caused by Get_Levelcodes
*
*  Known issues:
*  - File access in the main program runs via Qt / QFile routines, but in some header files still via the standard C routines or fopen_s.
*
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
#include  <QPushButton>
#include "tilelist.h"
#include "unitlist.h"
#include "buildable.h"
#include "building.h"
#include "replace.h"


//Global variables and constants:

QString                  Title = "History Line 1914-1918 Editor";
QString                  Author = "by Jan Knipperts";
QString                  Version = "RC 1";

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
QString                  Unitdat2_name = "/UNIT.DAT";
QString                  Cfg = "/CONFIG.CFG";               //Our config file

QString                  Actual_Level = "";
int                      Actual_Levelnum;

QImage                   MapImage;                          //I use a QImage as Screenbuffer to draw the map
QImage                   MapImageScaled;                    //Additional buffer for the scaled map image
QScrollArea              *scrollArea;


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

QWidget                  *replacedlg;
QImage                   tile_image1;
QLabel                   *Tile1;
QImage                   tile_image2;
QLabel                   *Tile2;
QPushButton              *ok_button;
unsigned char            r1,r2;

QRect                    screenrect;                        //A QRect to save the screen geometry and position windows accordingly.
QLabel                   *unit_name_text;                   //Text label to display unit name on mouse over
QLabel                   *buildable_unitname;

int                      Scale_factor = 2;                  //Default scaling factor for the old VGA bitmaps
unsigned char            selected_tile = 0x00;              //Define "Plains" as default tile
unsigned char            selected_unit = 0xFF;              //No unit is selected by default
int                      selected_building = -1;            //No building is selected by default
bool                     Res_loaded = false;                //to check if bitmaps have already been loaded into memory
bool                     summer = true;                     //set summer as default season for map ressources
bool                     no_tilechange = false;
bool                     changes = false;
bool                     already_saved = false;
bool                     replace_accepted = false;
bool                     grid_enabled = true;
bool                     show_warnings = true;
bool                     Player2 = true;


//now include the code to load game ressources etc.

#include "Lib.h"
#include "fin.h"
#include "codes.h"
#include "shp.h"
#include "units.h"
#include "other.h"




bool Check_levelcode(QString code)
{
    if (code.length() != 5)          //Wrong length for levelcode
        return false;

    int i;

    for (i = 0; i < code.length(); i++)
    {
        if (!code.at(i).isLetter())   //character is no letter
            return false;
        if (!code.at(i).toLatin1())   //character is no valid ASCII characer
            return false;
    }

    return true;

}




//=================== Main Window  ==========================


MainWindow::MainWindow()
//Creates Main Window and adds a scroll area to display maps

{
    createActions();
    createMenus();
    setWindowTitle(Title+" "+Author+" - Version: "+Version);

    //initialize images and the scroll area
    MapImage = QImage(); //Create a new QImage object for the map image
    MapImageScaled = QImage(); //100,100,QImage::Format_RGB16); //Create a scaled version of it

    QLabel *BetaWarning = new QLabel();
    BetaWarning->setText("<br> <br> <br><font color='white'>This is a Beta Version! Warning of bugs and drastic changes between versions. </font> <br> <br>"
                         "<font color='black'>Greetings to the dosreloaded community and thanks to everyone who supports this project. <br>"
                         "My special thanks go to llm for his help in reverse engineering the game files and to tbc21 for beta testing.</font>");

    scrollArea = new(QScrollArea);
    scrollArea->setBackgroundRole(QPalette::Dark);
    scrollArea->setWidget(BetaWarning);
    scrollArea->setVisible(true);

    setCentralWidget(scrollArea);


}



void MainWindow::closeEvent(QCloseEvent *event)
//Own closeEvent handler, primary to make sure allocated memory will be properly released

{
    if ((Map.loaded == true) && (changes == true))
    {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, Title, "There are unsaved changes to the map. Do you want to save them?",
                                      QMessageBox::Yes|QMessageBox::No);
        if (reply == QMessageBox::Yes)
            Save();
    }


    Release_Buffers();
    event->accept();
}

void MainWindow::mouseDoubleClickEvent( QMouseEvent *event )
{
    //Double Click to delete unit on current field
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

                //Transport unit?

                if ((Map.data[(field_pos*2)+1] == 0x2C) ||
                    (Map.data[(field_pos*2)+1] == 0x2D) ||
                    (Map.data[(field_pos*2)+1] == 0x34) ||
                    (Map.data[(field_pos*2)+1] == 0x35) ||
                    (Map.data[(field_pos*2)+1] == 0x3E) ||
                    (Map.data[(field_pos*2)+1] == 0x3F))
                    Correct_building_record_from_map(); //fix building data record


                Map.data[(field_pos*2)+1] = 0xFF;
                Redraw_Field(hx,hy,selected_tile,0xFF);
                MapImageScaled = MapImage.scaled(MapImage.width()*Scale_factor,MapImage.height()*Scale_factor); //Create a scaled version of it
                if(showgridAct->isChecked()) ShowGrid();  //redraw the grid if enabled
                Draw_Hexagon(hx,hy,QPen(Qt::red, 1),&MapImageScaled,true,true); //redraw the frame

                QLabel *imageLabel = new QLabel;     //Create a scroll area to display the map
                imageLabel->setPixmap(QPixmap::fromImage(MapImageScaled));
                scrollArea->setWidget(imageLabel);

                scrollArea->horizontalScrollBar()->setValue(pos_x); //Reset the scrollArea to last position
                scrollArea->verticalScrollBar()->setValue(pos_y);
                changes = true; //There are unsaved changes now
            }
        }
    }
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
                unsigned char old_tile = Map.data[field_pos*2];
                unsigned char old_unit = Map.data[(field_pos*2)+1];


                if ((!no_tilechange) &&  (Map.data[field_pos*2] != selected_tile))
                {                   
                    Map.data[field_pos*2] = (unsigned char) selected_tile;
                    changes = true; //There are unsaved changes now

                    if (show_warnings)
                    {
                        if (((selected_tile >= 0x12) && (selected_tile <= 0x14)) ||
                            ((selected_tile >= 0x09) && (selected_tile <= 0x0B)))
                        {
                            QMessageBox              Warning;
                            Warning.warning(this,"Warning:","Attention! Building parts of factories and depots that do not have an associated entrance and are not arranged as intended can still be opened in the game and then contain random garbage data.");
                            Warning.setFixedSize(500,200);
                        }
                    }
                }


                if ((selected_unit != 0xFF) && (Map.data[(field_pos*2)+1] != selected_unit))
                {

                    Map.data[(field_pos*2)+1] = (unsigned char) selected_unit;
                    changes = true; //There are unsaved changes now               

                    if (show_warnings)
                    {
                        QString Partname = QString::fromStdString(char2string(Partdat.name[selected_tile],8));
                        bool valid_terrain = TRUE;

                        int unit;
                        if (selected_unit != 0xFF)
                            unit = selected_unit / 2;
                        else
                            unit = Map.data[(field_pos*2)+1] / 2;

                        if (unit > Num_Units) unit = unit-Num_Units;

                        if (getbit(Unit_accessible_terrain[unit],0) == 0) //Deep Water is not accessible by unit
                            if (Partname.contains("SSSEA")) valid_terrain = FALSE;

                        if (getbit(Unit_accessible_terrain[unit],1) == 0) //Railroad tracks are not accessible by unit
                            if (Partname.contains("SRAIL")) valid_terrain = FALSE;

                        if (getbit(Unit_accessible_terrain[unit],2) == 0) //shallow water is not accessible by unit
                            if (Partname.contains("SCOAS")) valid_terrain = FALSE;

                        if (getbit(Unit_accessible_terrain[unit],3) == 0) //Trenches are not accessible by unit
                            if (Partname.contains("SWALL")) valid_terrain = FALSE;

                        if (getbit(Unit_accessible_terrain[unit],4) == 0) //Plains and road/bridge are not accessible by unit
                            if ((Partname.contains("SPLAI")) || (Partname.contains("SSTRE"))) valid_terrain = FALSE;

                        if (getbit(Unit_accessible_terrain[unit],5) == 0) //Forest is not accessible by unit
                            if (Partname.contains("SFORE")) valid_terrain = FALSE;

                        if (getbit(Unit_accessible_terrain[unit],6) == 0) //Mountains and narrow bridges are not accessible by unit
                            if (Partname.contains("SMOUN")) valid_terrain = FALSE;

                        if ((((selected_tile == 0x01) || (selected_tile == 0x02)) ||
                             ((selected_tile >= 0x0C) && (selected_tile <= 0x11))) && (selected_unit != 0xFF))
                            valid_terrain = FALSE;

                        if (!valid_terrain)
                        {
                            QMessageBox              Warning;
                            Warning.warning(this,"Warning:","You have placed a unit on terrain where the game does not provide for it. This can lead to glitches and errors when playing the map in game.");
                            Warning.setFixedSize(500,200);
                        }
                    }
                }

                if (((old_tile == 0x01) || (old_tile == 0x02)) ||       //If a building has been set or modified....
                    ((old_tile >= 0x0C) && (old_tile <= 0x11)) ||
                    ((old_unit == 0x2C) ||
                     (old_unit  == 0x2D) ||
                     (old_unit  == 0x34) ||
                     (old_unit  == 0x35) ||
                     (old_unit  == 0x3E) ||
                     (old_unit  == 0x3F)) ||
                    ((selected_tile == 0x01) || (selected_tile == 0x02)) ||
                    ((selected_tile >= 0x0C) && (selected_tile <= 0x11)) ||
                    ((selected_unit == 0x2C) ||
                     (selected_unit  == 0x2D) ||
                     (selected_unit  == 0x34) ||
                     (selected_unit  == 0x35) ||
                     (selected_unit  == 0x3E) ||
                     (selected_unit  == 0x3F)))
                    Correct_building_record_from_map(); //...Correct the building data record in memory

                Redraw_Field(hx,hy,Map.data[(field_pos*2)],Map.data[(field_pos*2)+1]);
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


            if ((hx > (Map.width-1)) || (hy > (Map.height-1)))  //Is the field on the map?
                return;

            int field_pos = (hy*Map.width)+hx;

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


                if (building_window == NULL)
                {
                  selected_building = Get_Buildings_info(field_pos);
                  Create_building_configuration_window();
                }
                else
                {
                  building_window->close();
                  selected_building = Get_Buildings_info(field_pos);
                  Create_building_configuration_window();
                }
            }
            else
            {


                if  (((selected_tile == 0x01) || (selected_tile == 0x02)) ||
                     ((selected_tile >= 0x0C) && (selected_tile <= 0x11)) ||
                    (selected_tile == 0x15))
                {

                    if ((selected_tile == 0x01) || (selected_tile == 0x02))
                    {
                        Change_Mapdata(hx,hy,selected_tile,0xFF);
                        Change_Mapdata(hx-1,hy+(hx%2),0x05,0xFF);
                        Change_Mapdata(hx,hy+1,0x03,0xFF);
                        Change_Mapdata(hx+1,hy+(hx%2),0x07,0xFF);
                        Change_Mapdata(hx-1,hy+(hx%2)+1,0x06,0xFF);
                        Change_Mapdata(hx,hy+2,0x04,0xFF);
                        Change_Mapdata(hx+1,hy+(hx%2)+1,0x08,0xFF);
                    }

                    if ((selected_tile >= 0x0C) && (selected_tile <= 0x0E))
                    {
                        Change_Mapdata(hx,hy,selected_tile,0xFF);

                        if (hx%2 == 1)
                        {
                            Change_Mapdata(hx-1,hy,0x09,0xFF);
                            Change_Mapdata(hx-1,hy+1,0x0A,0xFF);
                        }
                        else
                        {
                            Change_Mapdata(hx-1,hy-1,0x09,0xFF);
                            Change_Mapdata(hx-1,hy,0x0A,0xFF);
                        }
                        Change_Mapdata(hx-2,hy,0x0B,0xFF);
                    }

                    if ((selected_tile >= 0x0F) && (selected_tile <= 0x11))
                    {
                        Change_Mapdata(hx,hy,selected_tile,0xFF);
                        Change_Mapdata(hx-1,hy+(hx%2),0x13,0xFF);
                        Change_Mapdata(hx,hy+1,0x12,0xFF);
                        Change_Mapdata(hx+1,hy+(hx%2),0x14,0xFF);
                    }

                    if (selected_tile == 0x15)
                    {
                        Change_Mapdata(hx,hy,selected_tile,0xFF);
                        Change_Mapdata(hx-1,hy+(hx%2),0x17,0xFF);
                        Change_Mapdata(hx,hy+1,0x16,0xFF);
                        Change_Mapdata(hx+1,hy+(hx%2),0x18,0xFF);
                    }

                    Correct_building_record_from_map(); //...Correct the building data record in memory
                }
            }

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
            Errormsg.critical(this,"Error","Failed to load bitmaps from the game!");
            Errormsg.setFixedSize(500,200);
            return;
        }

    }

    if ((Map.loaded == true) && (changes == true))
    {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, Title, "There are unsaved changes to the map. Do you want to save them?",
                                      QMessageBox::Yes|QMessageBox::No);
        if (reply == QMessageBox::Yes)
            Save();
    }


    if (Map.data != NULL) free(Map.data);


    Map.width = 16; //Set default width and height
    Map.height = 16;
    Map.data_size = ((Map.width+1) * (Map.height+1)) * 2;


    if ((Map.data = (unsigned char*)malloc(Map.data_size)) == NULL)
    {
        QMessageBox              Errormsg;
        Errormsg.critical(this,"Error","Memory allocation error!");
        Errormsg.setFixedSize(500,200);
        return;
    }
    else
    {
        unsigned char tile;

        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, Title, "Do you want to create an ocean map?",
                                      QMessageBox::Yes|QMessageBox::No);
        if (reply == QMessageBox::Yes)
        {
            tile = 0x30;
        }
        else
        {
            tile = 0x00;
        }

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
                  Map.data[o] =  tile;
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
        QLabel *imageLabel = new QLabel;     //Update the scrollArea
        imageLabel->setPixmap(QPixmap::fromImage(MapImageScaled));
        if (scrollArea == NULL) scrollArea = new(QScrollArea);
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

        Building_stat.num_HQ = 0;
        Building_stat.num_F = 0;
        Building_stat.num_D = 0;
        Building_stat.num_T = 0;
        Building_stat.num_buildings = 0;
        if (Building_info != NULL) free(Building_info);
        if (SHP.buildings != NULL) free(SHP.buildings);
        Building_info = (Building_data_ext*) malloc(sizeof(Building_data_ext));
        memset(SHP.can_be_built,0,sizeof(SHP.can_be_built));
        SHP.buildings = 0;

        changes = true;
        already_saved = false;
        Actual_Level = "";
        Actual_Levelnum = -1;
        setWindowTitle(Title+" "+Author+" - Version: "+Version);
        Player2 = true;
        maptypeAct->setChecked(true);
    }
}


void MainWindow::Open_Map()
{
    QString                  C_Filename1;
    QString                  C_Filename2;
    QMessageBox              Errormsg;

    if (!Map_file.isEmpty() && !Map_file.isNull())
    {

        if (!Res_loaded)
        {
          if (Load_Ressources() != 0)
          {
              QMessageBox              Errormsg;
              Errormsg.critical(this,"Error","Failed to load bitmaps from the game!");
              Errormsg.setFixedSize(500,200);
              return;
          }
        }

        if (Load_Map() == 0)
        {                  
            if (!summer)
            {
                C_Filename1 = (GameDir + Partlib_W_name); //Create C style filenames for use of stdio
                C_Filename1.replace("/", "\\");
                C_Filename2 = (GameDir + Partdat_W_name);
                C_Filename2.replace("/", "\\");
            }
            else  //summer
            {
                C_Filename1 = (GameDir + Partlib_S_name); //Create C style filenames for use of stdio
                C_Filename1.replace("/", "\\");
                C_Filename2 = (GameDir + Partdat_S_name);
                C_Filename2.replace("/", "\\");
            }
            Load_Part_files(C_Filename1.toStdString().data(),C_Filename2.toStdString().data()); //Load correct season graphics

            if (Player2)
                maptypeAct->setChecked(true);
            else
                maptypeAct->setChecked(false);

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

            if (showunitwindowAct->isChecked() == true)
            {
                if (unit_selection == NULL)
                    Create_Unitselection_window();
                else
                    unit_selection->show();
            }

            changes = false;
            already_saved = true;
            Check_used_tiles();
            if (Actual_Level != "")
                setWindowTitle(Title+" editing map "+Actual_Level);
            else
                setWindowTitle(Title+" "+Author+" - Version: "+Version);

        }
    }
}

void MainWindow::open_diag()
{
    if ((Map.loaded == true) && (changes == true))
    {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, Title, "There are unsaved changes to the map. Do you want to save them?",
                                      QMessageBox::Yes|QMessageBox::No);
        if (reply == QMessageBox::Yes)
          Save();
    }

    Map_file = QFileDialog::getOpenFileName(this,
                                            tr("Open History Line 1914-1918 map file"),MapDir, tr("HL map files (*.fin)"));

    Open_Map();

}


void MainWindow::open_by_code_diag()
{
    if ((Map.loaded == true) && (changes == true))
    {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, Title, "There are unsaved changes to the map. Do you want to save them?",
                                      QMessageBox::Yes|QMessageBox::No);
        if (reply == QMessageBox::Yes)
          Save();
    }

    if (!Res_loaded)
    {
        if (Load_Ressources() != 0)
        {
          QMessageBox              Errormsg;
          Errormsg.critical(this,"Error","Failed to load bitmaps from the game!");
          Errormsg.setFixedSize(500,200);
          return;
        }
    }

    bool ok;


    QString levelcode = QInputDialog::getItem(this, tr("Open map by levelcode:"),
                                              "Please select a map:", Levelcode.Codelist, 0, false, &ok,Qt::Tool);

    if (ok && !levelcode.isEmpty())
    {
        if (!Check_levelcode(levelcode))
        {
          QMessageBox   Errormsg;
          Errormsg.critical(this,"","The selected levelcode is invalid! Are the game files corrupted?");
          Errormsg.setFixedSize(500,200);
            return;
        }

        int i;
        int fnum;
        fnum = 0;

        for (i=0;i < Levelcode.Codelist.count(); i++)
        {
            if (QString::compare(Levelcode.Codelist[i], levelcode, Qt::CaseInsensitive) == 0)
            {
              fnum = i;
              break;
            }
        }

        if (fnum < 10)
            Map_file = "0"+QString::number(fnum);
        else
            Map_file = QString::number(fnum);

        Map_file = Map_file+".fin" ;
        Map_file = MapDir+"/"+Map_file;
        Map_file.replace("/'", "\\'");
        Open_Map();
    }
}


void MainWindow::save_diag()
{
    if (Map.loaded == true)
        Save();
    else
    {
        QMessageBox              Errormsg;
        Errormsg.warning(this,"","There's nothing I could save.... Why don't you load a map first or create a new one?");
        Errormsg.setFixedSize(500,200);
    }

}


void MainWindow::saveas_diag()
{
    if (Map.loaded == true)
    {
        already_saved = false;
        Save();
        Check_used_tiles();
    }
    else
    {
        QMessageBox              Errormsg;
        Errormsg.warning(this,"","There's nothing I could save.... Why don't you load a map first or create a new one?");
        Errormsg.setFixedSize(500,200);
    }
}



void MainWindow::grid_diag()
{
    if(showgridAct->isChecked())
    {
        ShowGrid();  //Draw a Hexfield-Grid
        grid_enabled = true;
    }
    else
    {
        MapImageScaled = MapImage.scaled(MapImage.width()*Scale_factor,MapImage.height()*Scale_factor);
        grid_enabled = false;
    }

    QLabel *imageLabel = new QLabel;     //Update the scrollArea
    imageLabel->setPixmap(QPixmap::fromImage(MapImageScaled));
    if (scrollArea == NULL) scrollArea = new(QScrollArea);
    scrollArea->setWidget(imageLabel);
}


void MainWindow::statistics_diag()
{

    //Get number of used terrain tiles and units
    int parts = 0;
    int upper_parts = 0;
    int units = 0;

    int g_hq = 0;
    int g_f = 0;
    int g_d = 0;
    int g_res = 0;
    int g_units = 0;

    int f_hq = 0;
    int f_f = 0;
    int f_d = 0;
    int f_res = 0;
    int f_units = 0;

    int n_f = 0;
    int n_d = 0;
    int n_res = 0;
    int n_units = 0;



    unsigned char used_parts[Num_Parts];
    memset(&used_parts,0,sizeof(used_parts));
    unsigned char used_units[Num_Units];
    memset(&used_units,0,sizeof(used_units));

    for (int offset = 0; offset < (Map.width*Map.height)*2; offset += 2)
    {
        if (Map.data[offset] != 0xAE)
        {
            used_parts[Map.data[offset]] = 1;
        }
        if (Map.data[offset+1] != 0xFF)
        {
            if ((Map.data[offset+1] % 2) != 1)
              g_units++;
            else
              f_units++;
            used_units[Map.data[offset+1]/2] = 1;
        }


    }
    for (int i = 0; i < Num_Parts; i++)
        if (used_parts[i] == 1)
        {
            parts++;
            if (i > 24) upper_parts++;
        }

    for (int i = 0; i < Num_Units; i++)
        if (used_units[i] == 1) units++;

    for (int i = 0; i < Building_stat.num_buildings; i++)
    {
        if (Building_info[i].Properties->Owner == 0)
        {
            if (Building_info[i].Properties->Type == 0)
              g_hq++;
            if (Building_info[i].Properties->Type == 1)
              g_f++;
            if (Building_info[i].Properties->Type == 2)
              g_d++;

            g_res += Building_info[i].Properties->Resources;
            for (int i1 = 0; i1 < 6; i1++)
              if (Building_info[i].Properties->Units[i1] != 0xFF) g_units++;  //Add units inside buildings
        }
        if (Building_info[i].Properties->Owner == 1)
        {
            if (Building_info[i].Properties->Type == 0)
              f_hq++;
            if (Building_info[i].Properties->Type == 1)
              f_f++;
            if (Building_info[i].Properties->Type == 2)
              f_d++;

            f_res += Building_info[i].Properties->Resources;
            for (int i1 = 0; i1 < 6; i1++)
              if (Building_info[i].Properties->Units[i1] != 0xFF) f_units++;  //Add units inside buildings
        }
        if (Building_info[i].Properties->Owner == 2)
        {
            if (Building_info[i].Properties->Type == 1)
              n_f++;
            if (Building_info[i].Properties->Type == 2)
              n_d++;

            n_res += Building_info[i].Properties->Resources;
            for (int i1 = 0; i1 < 7; i1++)
              if (Building_info[i].Properties->Units[i1] != 0xFF) n_units++;  //Add units inside buildings
        }
    }



    QString numbersstr =
        "Map size: "+QString::number(Map.width)+"x"+QString::number(Map.height)+"\n"+
        "Different terrain tiles used: "+QString::number(parts)+"\n"+
        "Extended terrain tiles used: "+QString::number(upper_parts)+"\n"+
        "Different units used: "+QString::number(units)+"\n\n"+

                         "Germany: \n"
                         "Units: "+QString::number(g_units)+"\n"+
                         "Resource income per turn: "+QString::number(g_res)+"\n"+
                         "Buildings: "+QString::number((g_hq+g_f+g_d))+"\n"+
                         " - Headquarters: "+QString::number(g_hq)+"\n"+
                         " - Factories: "+QString::number(g_f)+"\n"+
                         " - Depots: "+QString::number(g_d)+"\n"+
                         "\n"+
                         "France: \n"
                         "Units: "+QString::number(f_units)+"\n"+
                         "Resource income per turn: "+QString::number(f_res)+"\n"+
                         "Buildings: "+QString::number((f_hq+f_f+f_d))+"\n"+
                         " - Headquarters: "+QString::number(f_hq)+"\n"+
                         " - Factories: "+QString::number(f_f)+"\n"+
                         " - Depots: "+QString::number(f_d)+"\n"+
                         "\n"+
                         "Neutral: \n"
                         "Units: "+QString::number(n_units)+"\n"+
                         "Resources: "+QString::number(n_res)+"\n"+
                         "Buildings: "+QString::number(n_f+n_d)+"\n"+
                         " - Factories: "+QString::number(n_f)+"\n"+
                         " - Depots: "+QString::number(n_d)+"\n";



    QMessageBox              Info;
    Info.information(this,"Information about your map:",numbersstr);
    Info.setFixedSize(500,200);


    Check_used_tiles();
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

    if (!GameDir.isEmpty() && !GameDir.isNull())
    {
        if (!Check_for_game_files())
        {
            QMessageBox              Errormsg;
            Errormsg.warning(this,"","I cannot find the required game files in the selected directory!");
            Errormsg.setFixedSize(500,200);
        }
        else
        {
            QFile cfgFile(dir.currentPath()+Cfg);
            cfgFile.open(QIODevice::WriteOnly);

            if (!cfgFile.isOpen())
            {
                QMessageBox              Errormsg;
                Errormsg.warning(this,"","I cannot save the configuration file!");
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
        &ok,
        Qt::Tool);

    if (ok)
    {
        if (Scale_factor < 1) Scale_factor = 1;

        MapImageScaled = MapImage.scaled(MapImage.width()*Scale_factor,MapImage.height()*Scale_factor);
        if(showgridAct->isChecked()) ShowGrid();  //redraw the grid if enabled

        QLabel *imageLabel = new QLabel;
        imageLabel->setPixmap(QPixmap::fromImage(MapImageScaled));
        scrollArea->setWidget(imageLabel);

        //Scale and update the child window contents

        if (showunitwindowAct->isChecked() == true)
        {
            UnitListImageScaled = UnitListImage.scaled(UnitListImage.width()*Scale_factor,UnitListImage.height()*Scale_factor);
            QLabel *imageLabel1 = new QLabel;
            imageLabel1->setPixmap(QPixmap::fromImage(UnitListImageScaled));
            unitscrollArea->setWidget(imageLabel1);
            unit_selection->update();
        }

        if (showtilewindowAct->isChecked() == true)
        {
            TileListImageScaled = TileListImage.scaled(TileListImage.width()*Scale_factor,TileListImage.height()*Scale_factor);
            QLabel *imageLabel2 = new QLabel;
            imageLabel2->setPixmap(QPixmap::fromImage(TileListImageScaled));
            tilescrollArea->setWidget(imageLabel2);
            tile_selection->update();
        }


    }

}



void MainWindow::add_diag()
{
    QString Codefile,Comfile,Newfile;

    if (Map.loaded == true)
    {
        Check_used_tiles();

        bool ok;
        QString levelcode = QInputDialog::getText(this, tr("Add map to game"),
                                                  tr("Enter a levelcode for your map (5 letters):"), QLineEdit::Normal,
                                                  "", &ok,Qt::Tool);
        if (ok && !levelcode.isEmpty())
        {
            if (!Check_levelcode(levelcode))
            {
                QMessageBox   Errormsg;
                Errormsg.critical(this,"","Code must be five letters to work with the game.");
                Errormsg.setFixedSize(500,200);
                return;
            }

            if (Levelcode_exists(levelcode))
            {
                QMessageBox   Errormsg;
                Errormsg.critical(this,"","Level already exists. Please choose another code for it.");
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
                Errormsg.critical(this,"","There are too many maps in the directory. The game can handle a maximum of 99.");
                Errormsg.setFixedSize(500,200);
                return;
            }

            int filenumber = QString((QString) maps[maps.size()-1][0]+maps[maps.size()-1][1]).toInt();


            if (Levelcode.Number_of_levels != (filenumber+1))
            {
                QMessageBox              Errormsg;
                Errormsg.warning(this,"","There are "+QString::number((filenumber+1))+" valid named map-files in the MAP subdirectory, but "
                                               +QString::number(Levelcode.Number_of_levels)+" maps stored in the game's Code.dat file. "+
                                               "Please clean up the directory first.");
                Errormsg.setFixedSize(500,200);
                return;
            }

            filenumber++;
            Map_file = QString::number(filenumber);
            Map_file = Map_file+".fin" ;
            Map_file = MapDir+"/"+Map_file;
            Map_file.replace("/'", "\\'");
            if (Save_Mapdata(Map_file.toStdString().data()) != 0) //Create new .fin file for this map.
            {
                QMessageBox   Errormsg;
                Errormsg.critical(this,"","Failed to create "+Map_file);
                Errormsg.setFixedSize(500,200);
                return;
            }

            QString SHPfile;
            SHPfile = Map_file;
            SHPfile.replace(".fin",".shp").replace(".FIN",".SHP");
            if (Create_shp(SHPfile.toStdString().data()) != 0)
            {
                QMessageBox              Errormsg;
                Errormsg.warning(this,"","I cannot save the building data in "+SHPfile);
                Errormsg.setFixedSize(500,200);
            }

            Codefile = (GameDir + Code_name); //Create a C style filename for use of stdio
            Codefile.replace("/'", "\\'");

            if (Add_map(Codefile.toStdString().data(), levelcode) != 0)
            {
                QMessageBox   Errormsg;
                Errormsg.critical(this,"","Can't write data for the new map to CODES.DAT");
                Errormsg.setFixedSize(500,200);
                return;
            }


            if (Player2 == false)
            {
                bool            ok;

                QStringList     items;

                items << "Type I"
                      << "Type II"
                    << "Type III"
                    << "Type IV";

                QString item = QInputDialog::getItem(this, tr("Type of computer opponent"),
                                                 tr("You have configured your map as a single player map. Please select the type of computer opponent (.COM file) for your map:"), items, 0, false, &ok,Qt::Tool);
                if (ok && !item.isEmpty())
                {

                    if (item == "Type I"){Comfile = "00.COM";}
                    if (item == "Type II"){Comfile = "21.COM";}
                    if (item == "Type III"){Comfile = "45.COM";}
                    if (item == "Type IV"){Comfile = "22.COM";}

                    Comfile = MapDir+"/"+Comfile;
                    Comfile.replace("/'", "\\'");

                    if (!QFile::exists(Comfile))
                    {
                        Comfile = Comfile.toLower();
                        if (!QFile::exists(Comfile))
                        {
                            QMessageBox   Errormsg;
                            Errormsg.critical(this,"","File "+Comfile+" not found!");
                            Errormsg.setFixedSize(500,200);
                            return;
                        }
                    }

                    Newfile = Map_file;
                    Newfile.replace(".fin",".com").replace(".FIN",".COM");
                    QFile::copy(Comfile, Newfile);

                    if (!QFile::exists(Newfile))
                    {
                        QMessageBox   Errormsg;
                        Errormsg.critical(this,"","Failed to create "+Newfile+"!");
                        Errormsg.setFixedSize(500,200);
                        return;
                    }
                }
            }

            Actual_Level = levelcode;
            changes = false;
            already_saved = true;
            setWindowTitle(Title+" "+Actual_Level);

        }
    }
    else
    {
        QMessageBox Errormsg;
        Errormsg.warning(this,"","There's nothing I could add to the game.... Why don't you load a map first or create a new one?");
        Errormsg.setFixedSize(500,200);
    }
}


void MainWindow::remove_diag()
{
    QString R_SHPfile, R_Mapfile, R_Comfile, R_Codefile, R_Hifile;
    int old_maxlevel;

    if (!Res_loaded)
    {
        if (Load_Ressources() != 0)
        {
            QMessageBox              Errormsg;
            Errormsg.critical(this,"Error","Failed to load bitmaps from the game!");
            Errormsg.setFixedSize(500,200);
            return;
        }
    }

    bool ok;


    QString R_levelcode = QInputDialog::getItem(this, tr("Remove map from game"),
                                              "Which map should be removed from the game?", Levelcode.Codelist, 0, false, &ok,Qt::Tool);

    if (ok && !R_levelcode.isEmpty())
    {
        if (!Check_levelcode(R_levelcode))
        {
            QMessageBox   Errormsg;
            Errormsg.critical(this,"","The selected levelcode is invalid! Are the game files corrupted?");
            Errormsg.setFixedSize(500,200);
            return;
        }

        int i;
        int fnum;
        fnum = 0;

        for (i=0;i < Levelcode.Codelist.count(); i++)
        {
            if (QString::compare(Levelcode.Codelist[i], R_levelcode, Qt::CaseInsensitive) == 0)
            {
                fnum = i;
                break;
            }
        }

        R_Codefile = (GameDir + Code_name); //Create a C style filename for use of stdio
        R_Codefile.replace("/'", "\\'");

        old_maxlevel = Levelcode.Number_of_levels;

        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, Title, "All references to the map "+R_levelcode+" will be removed from the game files and all files belonging to the map will be deleted. Are you sure?",
                                      QMessageBox::Yes|QMessageBox::No);
        if (reply == QMessageBox::No)
            return;

        if (Remove_map(R_Codefile.toStdString().data(), R_levelcode) != 0)
        {
            QMessageBox   Errormsg;
            Errormsg.critical(this,"","Failed to update the CODES.DAT file!");
            Errormsg.setFixedSize(500,200);
            return;
        }

        if (fnum < 10)
            R_Mapfile = "0"+QString::number(fnum);
        else
            R_Mapfile = QString::number(fnum);

        R_Mapfile = R_Mapfile+".fin" ;
        R_Mapfile = MapDir+"/"+R_Mapfile;
        R_Mapfile.replace("/'", "\\'");
        R_SHPfile = R_Mapfile;
        R_SHPfile.replace(".fin",".shp").replace(".FIN",".SHP");
        R_Comfile = R_Mapfile;
        R_Comfile.replace(".fin",".com").replace(".FIN",".COM");
        R_Hifile = R_Mapfile;
        R_Hifile.replace(".fin",".hi").replace(".FIN",".HI");

        if (QFile::exists(R_Mapfile))
            QFile::remove(R_Mapfile);
        if (QFile::exists(R_SHPfile))
            QFile::remove(R_SHPfile);
        if (QFile::exists(R_Comfile))
            QFile::remove(R_Comfile);
        if (QFile::exists(R_Hifile))
            QFile::remove(R_Hifile);


        //Alle umbenennen

        QString orig_file, new_file;

        if (fnum < old_maxlevel)
        {
            for (i = fnum+1; i <= old_maxlevel; i++)
            {
                if (i < 10)
                    orig_file = "0"+QString::number(i);
                else
                    orig_file = QString::number(i);

                if ((i-1) < 10)
                    new_file = "0"+QString::number(i-1);
                else
                    new_file = QString::number(i-1);


                orig_file = MapDir+"/"+orig_file;
                orig_file.replace("/'", "\\'");
                new_file = MapDir+"/"+new_file;
                new_file.replace("/'", "\\'");


                if (QFile::exists(orig_file+".FIN"))
                {
                    QFile::rename(orig_file+".FIN",new_file+".FIN");
                }
                else
                {
                    if (QFile::exists(orig_file+".fin"))
                        QFile::rename(orig_file+".fin",new_file+".fin");
                }

                if (QFile::exists(orig_file+".SHP"))
                {
                    QFile::rename(orig_file+".SHP",new_file+".SHP");
                }
                else
                {
                    if (QFile::exists(orig_file+".shp"))
                        QFile::rename(orig_file+".shp",new_file+".shp");
                }

                if (QFile::exists(orig_file+".COM"))
                {
                    QFile::rename(orig_file+".COM",new_file+".COM");
                }
                else
                {
                    if (QFile::exists(orig_file+".com"))
                        QFile::rename(orig_file+".com",new_file+".com");
                }

                if (QFile::exists(orig_file+".HI"))
                {
                    QFile::rename(orig_file+".HI",new_file+".HI");
                }
                else
                {
                    if (QFile::exists(orig_file+".hi"))
                        QFile::rename(orig_file+".hi",new_file+".hi");
                }
            }
        }

        if (Map.loaded == true)
        {
            if (Actual_Level == R_levelcode)
            {
                setWindowTitle(Title+" "+Author+" - Version: "+Version);
                Actual_Level = "";
                changes = true;
                already_saved = false;
           }
        }
    }

    return;
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

    if ((Map.width == 40) && (Map.height == 32)) current = 9;
    if ((Map.width == 40) && (Map.height == 40)) current = 10;

    if ((Map.width == 48) && (Map.height == 64)) current = 11;

    if ((Map.width == 64) && (Map.height == 24)) current = 12;
    if ((Map.width == 64) && (Map.height == 32)) current = 13;
    if ((Map.width == 64) && (Map.height == 48)) current = 14;


    QString item = QInputDialog::getItem(this, tr("Select map size"),
                                         tr("Map width x height = "), items, current, false, &ok,Qt::Tool);
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
        if (item == "40x32"){ width = 40;height = 32;}
        if (item == "40x40"){ width = 40;height = 40;}
        if (item == "48x64"){ width = 48;height = 64;}
        if (item == "64x24"){ width = 64;height = 24;}
        if (item == "64x32"){ width = 64;height = 32;}
        if (item == "64x48"){ width = 64;height = 48;}


        if ((data = (unsigned char*)malloc(((width+1) * (height+1) * 2))) == NULL)
        {
            QMessageBox  Errormsg;
            Errormsg.critical(this,"Error","Memory allocation error!");
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

        Add_building_positions(); //Correct building positions

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
    Errormsg.warning(this,"","Please load or create a map first.");
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
        Errormsg.critical(this,"Error","Faild to load summer/winter graphics from the game!");
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


void MainWindow::maptype_diag()
{
    if(maptypeAct->isChecked())
    {
        Player2 = true;
    }
    else
    {
        Player2 = false;
    }
}

void MainWindow::replace_diag()
{
    if (Map.loaded == true)
    {
        Create_replace_tile_diag();
    }
    else
    {
        QMessageBox Errormsg;
        Errormsg.warning(this,"","Please load or create a map first.");
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
        Errormsg.warning(this,"","Please load or create a map first.");
        Errormsg.setFixedSize(500,200);
    }
}


void MainWindow::warning_diag()
{
    if(warningAct->isChecked())
        show_warnings = TRUE;
    else
        show_warnings = FALSE;
}



void MainWindow::createActions()
{
    newAct = new QAction(tr("&New"), this);
    newAct->setShortcuts(QKeySequence::New);
    newAct->setStatusTip(tr("Create a new map"));
    connect(newAct, &QAction::triggered, this, &MainWindow::newFile_diag);

    openAct = new QAction(tr("&Open map by file"), this);
    openAct->setShortcuts(QKeySequence::Open);
    openAct->setStatusTip(tr("Open an existing map"));
    connect(openAct, &QAction::triggered, this, &MainWindow::open_diag);

    openbyCodeAct = new QAction(tr("Open map by levelcode"),this);
    openbyCodeAct->setStatusTip(tr("Open an existing map by its ingame levelcode"));
    connect(openbyCodeAct, &QAction::triggered, this, &MainWindow::open_by_code_diag);

    saveAct = new QAction(tr("&Save"), this);
    saveAct->setShortcuts(QKeySequence::Save);
    saveAct->setStatusTip(tr("Save the map to disk"));
    connect(saveAct, &QAction::triggered, this, &MainWindow::save_diag);

    saveasAct = new QAction(tr("Save map as..."), this);
    saveasAct->setStatusTip(tr("Save the map to a new file"));
    connect(saveasAct, &QAction::triggered, this, &MainWindow::saveas_diag);

    addtogameAct = new QAction(tr("&Add map to game"), this);
    addtogameAct->setStatusTip(tr("Adds your map to the game"));
    connect(addtogameAct, &QAction::triggered, this, &MainWindow::add_diag);

    removefromgameAct = new QAction(tr("&Remove map from game"), this);
    removefromgameAct->setStatusTip(tr("Removes a map from the game"));
    connect(removefromgameAct, &QAction::triggered, this, &MainWindow::remove_diag);

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

    replaceAct = new QAction(tr("Replace tile"),this);
    replaceAct->setStatusTip(tr("Replaces one tile with another"));
    connect(replaceAct,&QAction::triggered,this,&MainWindow::replace_diag);

    maptypeAct = new QAction(tr("Two-Player map"),this);
    maptypeAct->setStatusTip(tr("Sets Map type to single or two player map"));
    maptypeAct->setCheckable(true);
    maptypeAct->setChecked(true);
    connect(maptypeAct,&QAction::triggered,this,&MainWindow::maptype_diag);

    statisticsAct = new QAction(tr("Map info"),this);
    statisticsAct->setStatusTip(tr("Shows map statistics"));
    connect(statisticsAct,&QAction::triggered,this,&MainWindow::statistics_diag);

    buildableunitsAct = new QAction(tr("Set buildable units"),this);
    buildableunitsAct->setStatusTip(tr("Which units can be built in factories?"));
    connect(buildableunitsAct,&QAction::triggered,this,&MainWindow::buildable_units_diag);

    setPathAct = new QAction(tr("&Game path"),this);
    setPathAct->setStatusTip(tr("Set path to game resources"));
    connect(setPathAct,&QAction::triggered,this,&MainWindow::setPath_diag);

    setScaleFactorAct = new QAction(tr("&Scale factor"),this);
    setScaleFactorAct->setStatusTip(tr("Scaler used to enlarge/enhance low resolution bitmaps."));
    connect(setScaleFactorAct,&QAction::triggered,this,&MainWindow::setScale_diag);

    warningAct = new QAction(tr("Show warnings"), this);
    warningAct->setCheckable(true);
    warningAct->setChecked(true);
    warningAct->setStatusTip(tr("Issue a warning if the map cannot be displayed correctly in the game or could lead to errors in the game."));
    connect(warningAct,&QAction::triggered,this,&MainWindow::warning_diag);

}


void MainWindow::createMenus()
{
    fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(newAct);
    fileMenu->addAction(openAct);
    fileMenu->addAction(openbyCodeAct);
    fileMenu->addAction(saveAct);
    fileMenu->addAction(saveasAct);
    fileMenu->addAction(addtogameAct);
    fileMenu->addAction(removefromgameAct);
    fileMenu->addSeparator();
    fileMenu->addAction(exitAct);

    editMenu =  menuBar()->addMenu(tr("&Edit"));
    editMenu->addAction(mapresizeAct);
    editMenu->addAction(changeseasonAct);
    editMenu->addAction(maptypeAct);
    editMenu->addAction(replaceAct);
    editMenu->addAction(buildableunitsAct);
    editMenu->addSeparator();
    editMenu->addAction(showgridAct);
    editMenu->addAction(showtilewindowAct);
    editMenu->addAction(showunitwindowAct);
    editMenu->addAction(statisticsAct);

    configMenu = menuBar()->addMenu(tr("&Settings"));
    configMenu->addAction(setPathAct);
    configMenu->addAction(setScaleFactorAct);
    configMenu->addAction(warningAct);            
}



//========================== Event handling for child windows =================================


void tilelistwindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        int pos_x = tilescrollArea->horizontalScrollBar()->value();
        int pos_y = tilescrollArea->verticalScrollBar()->value();
        TileListImageScaled = TileListImage.scaled(TileListImage.width()*Scale_factor,TileListImage.height()*Scale_factor); //Restore original image
        QRect widgetRect = tilescrollArea->geometry();
        int fx = (event->pos().x()-widgetRect.left()+pos_x) / (Tilesize*Scale_factor); // Calc field position from mouse cords
        int fy = (event->pos().y()-widgetRect.top()+pos_y) / (Tilesize*Scale_factor);

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
        if (unitscrollArea->rect().contains(event->pos()))
        {
            int pos_x = unitscrollArea->horizontalScrollBar()->value();
            int pos_y = unitscrollArea->verticalScrollBar()->value();
            UnitListImageScaled = UnitListImage.scaled(UnitListImage.width()*Scale_factor,UnitListImage.height()*Scale_factor); //Restore original image
            QRect widgetRect = unitscrollArea->geometry();
            int fx = (event->pos().x()-widgetRect.left()+pos_x) / (Tilesize*Scale_factor); // Calc field position from mouse cords
            int fy = (event->pos().y()-widgetRect.top()+pos_y)/ (Tilesize*Scale_factor);
            int os = selected_unit;

            selected_unit = ((fy*10)+fx)*2;

            if (fy > 5)
              selected_unit = selected_unit-119;     //Calc correct unit number for french units

            if (selected_unit < Num_Units*2)
            {
              unit_name_text->setText(Unit_Name[selected_unit/2]);
              Draw_Hexagon(fx,fy,QPen(Qt::red, 1),&UnitListImageScaled,false,true);
            }
            else
            {
              selected_unit = os;
              return;
            }

            QLabel *label = new QLabel();
            label->setPixmap(QPixmap::fromImage(UnitListImageScaled));  //update the image

            unitscrollArea->setWidget(label);
            unit_selection->update();                           //Update the window contents
            unitscrollArea->horizontalScrollBar()->setValue(pos_x); //Reset the scrollArea to last position
            unitscrollArea->verticalScrollBar()->setValue(pos_y);
        }
    }

    if (event->button() == Qt::RightButton)
    {
        if (unitscrollArea->rect().contains(event->pos()))
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
}


void buildablewindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
       if (buildablescrollArea->rect().contains(event->pos()))
       {
         int pos_x = buildablescrollArea->horizontalScrollBar()->value();
         int pos_y = buildablescrollArea->verticalScrollBar()->value();
         QRect widgetRect = buildablescrollArea->geometry();
         int fx = (event->pos().x()-widgetRect.left()+pos_x) / (Tilesize*Scale_factor); // Calc field position from mouse cords
         int fy = (event->pos().y()-widgetRect.top()+pos_y) / (Tilesize*Scale_factor);

         int unit = ((fy*10)+fx);     //Calc correct unit number
         buildable_unitname->setText(Unit_Name[unit]);

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
        changes = true; //Now there are unsaved changes
       }
    }
}




void buildingwindow::mousePressEvent(QMouseEvent *event)
{
    QRect widgetRect = Building_ScrollArea->geometry();

  if ((event->button() == Qt::LeftButton) && (selected_building != -1))
    {
       //Is the mouse on the ScrollArea (The list of units in the building?)
       if (widgetRect.contains(event->pos()))
       {
       // QPoint mouse_pos = Building_ScrollArea->mapFromParent(event->pos());

        int fx = ((event->pos().x()-widgetRect.left()) / Scale_factor) / Tilesize;

        Building_info[selected_building].Properties->Units[fx] = selected_unit/2;

        QPainter painter(&Building_Image);
        QPen pen;

        pen.setWidth(Tilesize);
        pen.setColor(Qt::black);
        painter.setPen(pen);
        painter.drawPoint((fx*Tilesize)+(Tilesize/2),(Tilesize/2));
        painter.end();

        Draw_Unit(fx*Tilesize, 0,(Building_info[selected_building].Properties->Units[fx] * 6) , Building_info[selected_building].Properties->Owner+1, &Building_Image);
        Building_Image_Scaled = Building_Image.scaled(Building_Image.width()*Scale_factor,Building_Image.height()*Scale_factor); //Create a scaled version of it

        for (int i = 0; i < 7; i++)
        {
            QPainter painter(&Building_Image_Scaled);
            pen.setWidth(1);
            pen.setColor(Qt::white);
            painter.setPen(pen);
            QRect R((i*Tilesize)*Scale_factor,0,((i*Tilesize)+Tilesize)*Scale_factor,Building_Image_Scaled.height()-1);
            painter.drawRect(R);
            painter.end();
        }


        QLabel *bitmaplabel = new QLabel();
        bitmaplabel->setPixmap(QPixmap::fromImage(Building_Image_Scaled));
        Building_ScrollArea->setWidget(bitmaplabel);
        Building_ScrollArea->update();
        building_window->update();
        changes = true; //Now there are unsaved changes
       }
    }

    if ((event->button() == Qt::RightButton) && (selected_building != -1))
    {

       //Is the mouse on the ScrollArea (The list of units in the building?)
       if (widgetRect.contains(event->pos()))
       {
        int fx = ((event->pos().x()-widgetRect.left()) / Scale_factor) / Tilesize;

        Building_info[selected_building].Properties->Units[fx] = 0xFF;
        QPainter painter(&Building_Image);
        QPen pen;
        pen.setWidth(Tilesize);
        pen.setColor(Qt::black);
        painter.setPen(pen);
        painter.drawPoint((fx*Tilesize)+(Tilesize/2),(Tilesize/2));
        painter.end();

        Building_Image_Scaled = Building_Image.scaled(Building_Image.width()*Scale_factor,Building_Image.height()*Scale_factor); //Create a scaled version of it
        for (int i = 0; i < 7; i++)
        {
            QPainter painter(&Building_Image_Scaled);
            QPen pen;
            pen.setWidth(1);
            pen.setColor(Qt::white);
            painter.setPen(pen);
            QRect R((i*Tilesize)*Scale_factor,0,((i*Tilesize)+Tilesize)*Scale_factor,Building_Image_Scaled.height()-1);
            painter.drawRect(R);
            painter.end();
        }

        QLabel *bitmaplabel = new QLabel();
        bitmaplabel->setPixmap(QPixmap::fromImage(Building_Image_Scaled));
        Building_ScrollArea->setWidget(bitmaplabel);
        Building_ScrollArea->update();
        building_window->update();
        changes = true; //Now there are unsaved changes
       }
    }

}

void buildingwindow::closeEvent(QCloseEvent *event)
{
    if (selected_building != -1)
    {
        if (RessourceEdit->text().toInt() != Building_info[selected_building].Properties->Resources)
        Building_info[selected_building].Properties->Resources = RessourceEdit->text().toInt();
    }
    event->accept();
}


void update_replacewindow()
{
    tile_image1 = QImage(Tilesize,Tilesize, QImage::Format_RGB16); //Create a new QImage object
    tile_image1.fill(Qt::transparent);
    Draw_Part(0,0,r1,&tile_image1);
    tile_image2 = QImage(Tilesize,Tilesize, QImage::Format_RGB16); //Create a new QImage object
    tile_image2.fill(Qt::transparent);
    Draw_Part(0,0,r2,&tile_image2);

    tile_image1 = tile_image1.scaled(tile_image1.width()*Scale_factor,tile_image1.height()*Scale_factor); //scale it
    tile_image2 = tile_image2.scaled(tile_image2.width()*Scale_factor,tile_image2.height()*Scale_factor); //scale i
    Tile1->setPixmap(QPixmap::fromImage(tile_image1));
    Tile2->setPixmap(QPixmap::fromImage(tile_image2));
    replacedlg->update();
}

void replacewindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        QRect widgetRect = Tile1->geometry();

        if (widgetRect.contains(event->pos()))
        {
            r1 = selected_tile;
            update_replacewindow();
        }

        widgetRect = Tile2->geometry();

        if (widgetRect.contains(event->pos()))
        {
            r2 = selected_tile;
            update_replacewindow();
        }

    }
}


void replacewindow::closeEvent(QCloseEvent *event)
{
    if ((replace_accepted) && (r1 != r2))
    {
        int                      x,y,offset;
        field_info               fielddata;

        offset = 0;
        for (y = 0; y < Map.height; y++)
        {
            for (x = 0; x < Map.width; x++)
            {
                memcpy(&fielddata, Map.data + offset, sizeof(fielddata));
                if (fielddata.Part == r1)
                {
                    fielddata.Part = r2;
                    memcpy(Map.data + offset,&fielddata, sizeof(fielddata));
                }
                offset = offset + sizeof(Field);
            }
        }

        changes = true;
        MapImage.fill(Qt::transparent);
        Draw_Map(); //redraw the mapimage
        MapImageScaled = MapImage.scaled(MapImage.width()*Scale_factor,MapImage.height()*Scale_factor); //Create a scaled version of it
        if (grid_enabled) ShowGrid();  //redraw the grid if enabled
        QLabel *imageLabel = new QLabel;     //Create a scroll area to display the map
        imageLabel->setPixmap(QPixmap::fromImage(MapImageScaled));
        scrollArea->setWidget(imageLabel);
    }

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


